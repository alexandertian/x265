/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Author: Shazeb Nawaz Khan <shazeb@multicorewareinc.com>
 *         Steve Borho <steve@borho.org>
 *         Kavitha Sampas <kavitha@multicorewareinc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "TLibCommon/TComPic.h"
#include "lowres.h"
#include "mv.h"
#include "slicetype.h"
#include "bitstream.h"

using namespace x265;
namespace weightp {
struct Cache
{
    const int * intraCost;
    int         numPredDir;
    int         csp;
    int         hshift;
    int         vshift;
    int         lowresWidthInCU;
    int         lowresHeightInCU;
};

int sliceHeaderCost(wpScalingParam *w, int lambda, int bChroma)
{
    /* 4 times higher, because chroma is analyzed at full resolution. */
    if (bChroma)
        lambda *= 4;
    int denomCost = bs_size_ue(w[0].log2WeightDenom) * (2 - bChroma);
    return lambda * (10 + denomCost + 2 * (bs_size_se(w[0].inputWeight) + bs_size_se(w[0].inputOffset)));
}

/* make a motion compensated copy of lowres ref into mcout with the same stride.
 * The borders of mcout are not extended */
void mcLuma(pixel* mcout, Lowres& ref, const MV * mvs)
{
    int stride = ref.lumaStride;
    const int cuSize = 8;
    MV mvmin, mvmax;

    int cu = 0;

    for (int y = 0; y < ref.lines; y += cuSize)
    {
        int pixoff = y * stride;
        mvmin.y = (int16_t)((-y - 8) << 2);
        mvmax.y = (int16_t)((ref.lines - y - 1 + 8) << 2);

        for (int x = 0; x < ref.width; x += cuSize, pixoff += cuSize, cu++)
        {
            ALIGN_VAR_16(pixel, buf8x8[8 * 8]);
            intptr_t bstride = 8;
            mvmin.x = (int16_t)((-x - 8) << 2);
            mvmax.x = (int16_t)((ref.width - x - 1 + 8) << 2);

            /* clip MV to available pixels */
            MV mv = mvs[cu];
            mv = mv.clipped(mvmin, mvmax);
            pixel *tmp = ref.lowresMC(pixoff, mv, buf8x8, bstride);
            primitives.luma_copy_pp[LUMA_8x8](mcout + pixoff, stride, tmp, bstride);
        }
    }

    x265_emms();
}

/* use lowres MVs from lookahead to generate a motion compensated chroma plane.
 * if a block had cheaper lowres cost as intra, we treat it as MV 0 */
void mcChroma(pixel *      mcout,
              pixel *      src,
              int          stride,
              const MV *   mvs,
              const Cache& cache,
              int          height,
              int          width)
{
    /* the motion vectors correspond to 8x8 lowres luma blocks, or 16x16 fullres
     * luma blocks. We have to adapt block size to chroma csp */
    int csp = cache.csp;
    int bw = 16 >> cache.hshift;
    int bh = 16 >> cache.vshift;
    MV mvmin, mvmax;

    for (int y = 0; y < height; y += bh)
    {
        /* note: lowres block count per row might be different from chroma block
         * count per row because of rounding issues, so be very careful with indexing
         * into the lowres structures */
        int cu = y * cache.lowresWidthInCU;
        int pixoff = y * stride;
        mvmin.y = (int16_t)((-y - 8) << 2);
        mvmax.y = (int16_t)((height - y - 1 + 8) << 2);

        for (int x = 0; x < width; x += bw, cu++, pixoff += bw)
        {
            if (x < cache.lowresWidthInCU && y < cache.lowresHeightInCU)
            {
                MV mv = mvs[cu]; // lowres MV
                mv <<= 1;        // fullres MV
                mv.x >>= cache.hshift;
                mv.y >>= cache.vshift;

                /* clip MV to available pixels */
                mvmin.x = (int16_t)((-x - 8) << 2);
                mvmax.x = (int16_t)((width - x - 1 + 8) << 2);
                mv = mv.clipped(mvmin, mvmax);

                int fpeloffset = (mv.y >> 2) * stride + (mv.x >> 2);
                pixel *temp = src + pixoff + fpeloffset;

                int xFrac = mv.x & 0x7;
                int yFrac = mv.y & 0x7;
                if ((yFrac | xFrac) == 0)
                {
                    primitives.chroma[csp].copy_pp[LUMA_16x16](mcout + pixoff, stride, temp, stride);
                }
                else if (yFrac == 0)
                {
                    primitives.chroma[csp].filter_hpp[LUMA_16x16](temp, stride, mcout + pixoff, stride, xFrac);
                }
                else if (xFrac == 0)
                {
                    primitives.chroma[csp].filter_vpp[LUMA_16x16](temp, stride, mcout + pixoff, stride, yFrac);
                }
                else
                {
                    ALIGN_VAR_16(int16_t, imm[16 * (16 + NTAPS_CHROMA)]);
                    primitives.chroma[csp].filter_hps[LUMA_16x16](temp, stride, imm, bw, xFrac, 1);
                    primitives.chroma[csp].filter_vsp[LUMA_16x16](imm + ((NTAPS_CHROMA >> 1) - 1) * bw, bw, mcout + pixoff, stride, yFrac);
                }
            }
            else
            {
                primitives.chroma[csp].copy_pp[LUMA_16x16](mcout + pixoff, stride, src + pixoff, stride);
            }
        }
    }

    x265_emms();
}

/* Measure sum of 8x8 satd costs between source frame and reference
 * frame (potentially weighted, potentially motion compensated). We
 * always use source images for this analysis since reference recon
 * pixels have unreliable availability */
uint32_t weightCost(pixel *         fenc,
                    pixel *         ref,
                    pixel *         weightTemp,
                    int             stride,
                    const Cache &   cache,
                    int             width,
                    int             height,
                    wpScalingParam *w,
                    bool            bLuma)
{
    if (w)
    {
        /* make a weighted copy of the reference plane */
        int offset = w->inputOffset << (X265_DEPTH - 8);
        int weight = w->inputWeight;
        int denom = w->log2WeightDenom;
        int round = denom ? 1 << (denom - 1) : 0;
        int correction = IF_INTERNAL_PREC - X265_DEPTH; /* intermediate interpolation depth */
        int pwidth = ((width + 15) >> 4) << 4;

        primitives.weight_pp(ref, weightTemp, stride, stride, pwidth, height,
                             weight, round << correction, denom + correction, offset);
        ref = weightTemp;
    }

    uint32_t cost = 0;
    pixel *f = fenc, *r = ref;

    if (bLuma)
    {
        int cu = 0;
        for (int y = 8; y < height; y += 8, r += 8 * stride, f += 8 * stride)
        {
            for (int x = 8; x < width; x += 8, cu++)
            {
                int cmp = primitives.satd[LUMA_8x8](r + x, stride, f + x, stride);
                cost += X265_MIN(cmp, cache.intraCost[cu]);
            }
        }
    }
    else if (cache.csp == X265_CSP_I444)
        for (int y = 16; y < height; y += 16, r += 16 * stride, f += 16 * stride)
        {
            for (int x = 16; x < width; x += 16)
            {
                cost += primitives.satd[LUMA_16x16](r + x, stride, f + x, stride);
            }
        }

    else
        for (int y = 8; y < height; y += 8, r += 8 * stride, f += 8 * stride)
        {
            for (int x = 8; x < width; x += 8)
            {
                cost += primitives.satd[LUMA_8x8](r + x, stride, f + x, stride);
            }
        }

    x265_emms();
    return cost;
}

void analyzeWeights(TComSlice& slice, x265_param& param, wpScalingParam wp[2][MAX_NUM_REF][3])
{
    TComPicYuv *fencYuv = slice.getPic()->getPicYuvOrg();
    Lowres& fenc        = slice.getPic()->m_lowres;

    weightp::Cache cache;

    memset(&cache, 0, sizeof(cache));
    cache.intraCost = fenc.intraCost;
    cache.numPredDir = slice.isInterP() ? 1 : 2;
    cache.lowresWidthInCU = fenc.width >> 3;
    cache.lowresHeightInCU = fenc.lines >> 3;
    cache.csp = fencYuv->m_picCsp;
    cache.hshift = CHROMA_H_SHIFT(cache.csp);
    cache.vshift = CHROMA_V_SHIFT(cache.csp);

    /* Use single allocation for motion compensated ref and weight buffers */
    pixel *mcbuf = X265_MALLOC(pixel, 2 * fencYuv->getStride() * fencYuv->getHeight());
    if (!mcbuf)
        return;
    pixel *weightTemp = mcbuf + fencYuv->getStride() * fencYuv->getHeight();

    int lambda = (int)x265_lambda_tab[X265_LOOKAHEAD_QP];
    int curPoc = slice.getPOC();
    const float epsilon = 1.f / 128.f;

    int chromaDenom, lumaDenom, denom;
    chromaDenom = lumaDenom = 7;
    int numpixels[3];
    int w = ((fencYuv->getWidth()  + 15) >> 4) << 4;
    int h = ((fencYuv->getHeight() + 15) >> 4) << 4;
    numpixels[0] = w * h;
    numpixels[1] = numpixels[2] = numpixels[0] >> (cache.hshift + cache.vshift);

    for (int list = 0; list < cache.numPredDir; list++)
    {
        wpScalingParam *weights = wp[list][0];
        TComPic *refPic = slice.getRefPic(list, 0);
        Lowres& refLowres = refPic->m_lowres;
        int diffPoc = abs(curPoc - refPic->getPOC());

        /* prepare estimates */
        float guessScale[3], fencMean[3], refMean[3];
        for (int plane = 0; plane < 3; plane++)
        {
            SET_WEIGHT(weights[plane], false, 1, 0, 0);
            uint64_t fencVar = fenc.wp_ssd[plane] + !refLowres.wp_ssd[plane];
            uint64_t refVar  = refLowres.wp_ssd[plane] + !refLowres.wp_ssd[plane];
            guessScale[plane] = sqrt((float)fencVar / refVar);
            fencMean[plane] = (float)fenc.wp_sum[plane] / (numpixels[plane]) / (1 << (X265_DEPTH - 8));
            refMean[plane]  = (float)refLowres.wp_sum[plane] / (numpixels[plane]) / (1 << (X265_DEPTH - 8));
        }

        /* make sure both our scale factors fit */
        while (!list && chromaDenom > 0)
        {
            float thresh = 127.f / (1 << chromaDenom);
            if (guessScale[1] < thresh && guessScale[2] < thresh)
                break;
            chromaDenom--;
        }

        SET_WEIGHT(weights[1], false, 1 << chromaDenom, chromaDenom, 0);
        SET_WEIGHT(weights[2], false, 1 << chromaDenom, chromaDenom, 0);

        MV *mvs = NULL;

        for (int plane = 0; plane < 3; plane++)
        {
            denom = plane ? chromaDenom : lumaDenom;
            if (plane && !weights[0].bPresentFlag)
                break;

            /* Early termination */
            x265_emms();
            if (fabsf(refMean[plane] - fencMean[plane]) < 0.5f && fabsf(1.f - guessScale[plane]) < epsilon)
            {
                SET_WEIGHT(weights[plane], 0, 1 << denom, denom, 0);
                continue;
            }

            if (plane)
            {
                int scale = Clip3(0, 255, (int)(guessScale[plane] * (1 << denom) + 0.5f));
                if (scale > 127)
                    continue;
                weights[plane].inputWeight = scale;
            }
            else
            {
                weights[plane].setFromWeightAndOffset((int)(guessScale[plane] * (1 << denom) + 0.5f), 0, denom, !list);
            }

            int mindenom = weights[plane].log2WeightDenom;
            int minscale = weights[plane].inputWeight;
            int minoff = 0;

            if (!plane && diffPoc <= param.bframes + 1)
            {
                mvs = fenc.lowresMvs[list][diffPoc - 1];

                /* test whether this motion search was performed by lookahead */
                if (mvs[0].x != 0x7FFF)
                {
                    /* reference chroma planes must be extended prior to being
                     * used as motion compensation sources */
                    if (!refPic->m_bChromaPlanesExtended)
                    {
                        refPic->m_bChromaPlanesExtended = true;
                        TComPicYuv *refyuv = refPic->getPicYuvOrg();
                        int stride = refyuv->getCStride();
                        int width = refyuv->getWidth() >> cache.hshift;
                        int height = refyuv->getHeight() >> cache.vshift;
                        int marginX = refyuv->getChromaMarginX();
                        int marginY = refyuv->getChromaMarginY();
                        extendPicBorder(refyuv->getCbAddr(), stride, width, height, marginX, marginY);
                        extendPicBorder(refyuv->getCrAddr(), stride, width, height, marginX, marginY);
                    }
                }
                else
                    mvs = 0;
            }

            /* prepare inputs to weight analysis */
            pixel *orig;
            pixel *fref;
            int    stride;
            int    width, height;
            switch (plane)
            {
            case 0:
                orig = fenc.lowresPlane[0];
                stride = fenc.lumaStride;
                width = fenc.width;
                height = fenc.lines;
                fref = refLowres.lowresPlane[0];
                if (mvs)
                {
                    mcLuma(mcbuf, refLowres, mvs);
                    fref = mcbuf;
                }
                break;

            case 1:
                orig = fencYuv->getCbAddr();
                stride = fencYuv->getCStride();
                fref = refPic->getPicYuvOrg()->getCbAddr();

                /* Clamp the chroma dimensions to the nearest multiple of
                 * 8x8 blocks (or 16x16 for 4:4:4) since mcChroma uses lowres
                 * blocks and weightCost measures 8x8 blocks. This
                 * potentially ignores some edge pixels, but simplifies the
                 * logic and prevents reading uninitialized pixels. Lowres
                 * planes are border extended and require no clamping. */
                width =  ((fencYuv->getWidth()  >> 4) << 4) >> cache.hshift;
                height = ((fencYuv->getHeight() >> 4) << 4) >> cache.vshift;
                if (mvs)
                {
                    mcChroma(mcbuf, fref, stride, mvs, cache, height, width);
                    fref = mcbuf;
                }
                break;

            case 2:
                fref = refPic->getPicYuvOrg()->getCrAddr();
                orig = fencYuv->getCrAddr();
                stride = fencYuv->getCStride();
                width =  ((fencYuv->getWidth()  >> 4) << 4) >> cache.hshift;
                height = ((fencYuv->getHeight() >> 4) << 4) >> cache.vshift;
                if (mvs)
                {
                    mcChroma(mcbuf, fref, stride, mvs, cache, height, width);
                    fref = mcbuf;
                }
                break;

            default:
                X265_FREE(mcbuf);
                return;
            }

            uint32_t origscore = weightCost(orig, fref, weightTemp, stride, cache, width, height, NULL, !plane);
            if (!origscore)
            {
                SET_WEIGHT(weights[plane], 0, 1 << denom, denom, 0);
                continue;
            }

            uint32_t minscore = origscore;
            bool bFound = false;

            /* x264 uses a table lookup here, selecting search range based on preset */
            static const int scaleDist = 4;
            static const int offsetDist = 2;

            int startScale = Clip3(0, 127, minscale - scaleDist);
            int endScale   = Clip3(0, 127, minscale + scaleDist);
            for (int scale = startScale; scale <= endScale; scale++)
            {
                int deltaWeight = scale - (1 << mindenom);
                if (deltaWeight > 127 || deltaWeight <= -128)
                    continue;

                int curScale = scale;
                int curOffset = (int)(fencMean[plane] - refMean[plane] * curScale / (1 << mindenom) + 0.5f);
                if (curOffset < -128 || curOffset > 127)
                {
                    /* Rescale considering the constraints on curOffset. We do it in this order
                     * because scale has a much wider range than offset (because of denom), so
                     * it should almost never need to be clamped. */
                    curOffset = Clip3(-128, 127, curOffset);
                    curScale = (int)((1 << mindenom) * (fencMean[plane] - curOffset) / refMean[plane] + 0.5f);
                    curScale = Clip3(0, 127, curScale);
                }

                int startOffset = Clip3(-128, 127, curOffset - offsetDist);
                int endOffset   = Clip3(-128, 127, curOffset + offsetDist);
                for (int off = startOffset; off <= endOffset; off++)
                {
                    wpScalingParam wsp;
                    SET_WEIGHT(wsp, true, curScale, mindenom, off);
                    uint32_t s = weightCost(orig, fref, weightTemp, stride, cache, width, height, &wsp, !plane) +
                        sliceHeaderCost(&wsp, lambda, !!plane);
                    COPY4_IF_LT(minscore, s, minscale, curScale, minoff, off, bFound, true);

                    /* Don't check any more offsets if the previous one had a lower cost than the current one */
                    if (minoff == startOffset && off != startOffset)
                        break;
                }
            }

            /* Use a smaller luma denominator if possible */
            if (!(plane || list))
            {
                while (mindenom > 0 && !(minscale & 1))
                {
                    mindenom--;
                    minscale >>= 1;
                }
            }

            if (!bFound || (minscale == (1 << mindenom) && minoff == 0) || (float)minscore / origscore > 0.998f)
            {
                SET_WEIGHT(weights[plane], false, 1 << denom, denom, 0);
            }
            else
            {
                SET_WEIGHT(weights[plane], true, minscale, mindenom, minoff);
            }
        }

        if (weights[0].bPresentFlag)
        {
            // Make sure both chroma channels match
            if (weights[1].bPresentFlag != weights[2].bPresentFlag)
            {
                if (weights[1].bPresentFlag)
                    weights[2] = weights[1];
                else
                    weights[1] = weights[2];
            }
        }

        lumaDenom = weights[0].log2WeightDenom;
        chromaDenom = weights[1].log2WeightDenom;

        /* reset weight states */
        for (int ref = 1; ref < slice.getNumRefIdx(list); ref++)
        {
            SET_WEIGHT(wp[list][ref][0], false, 1 << lumaDenom, lumaDenom, 0);
            SET_WEIGHT(wp[list][ref][1], false, 1 << chromaDenom, chromaDenom, 0);
            SET_WEIGHT(wp[list][ref][2], false, 1 << chromaDenom, chromaDenom, 0);
        }
    }

    X265_FREE(mcbuf);
}
}

namespace x265 {
void weightAnalyse(TComSlice& slice, x265_param& param)
{
    wpScalingParam wp[2][MAX_NUM_REF][3];

    weightp::analyzeWeights(slice, param, wp);

    slice.setWpScaling(wp);

    if (param.logLevel >= X265_LOG_FULL)
    {
        char buf[1024];
        int p = 0;
        bool bWeighted = false;

        p = sprintf(buf, "poc: %d weights:", slice.getPOC());
        int numPredDir = slice.isInterP() ? 1 : 2;
        for (int list = 0; list < numPredDir; list++)
        {
            wpScalingParam* w = &wp[list][0][0];
            if (w[0].bPresentFlag || w[1].bPresentFlag || w[2].bPresentFlag)
            {
                bWeighted = true;
                p += sprintf(buf + p, " [L%d:R0 ", list);
                if (w[0].bPresentFlag)
                    p += sprintf(buf + p, "Y{%d/%d%+d}", w[0].inputWeight, 1 << w[0].log2WeightDenom, w[0].inputOffset);
                if (w[1].bPresentFlag)
                    p += sprintf(buf + p, "U{%d/%d%+d}", w[1].inputWeight, 1 << w[1].log2WeightDenom, w[1].inputOffset);
                if (w[2].bPresentFlag)
                    p += sprintf(buf + p, "V{%d/%d%+d}", w[2].inputWeight, 1 << w[2].log2WeightDenom, w[2].inputOffset);
                p += sprintf(buf + p, "]");
            }
        }

        if (bWeighted)
        {
            if (p < 80) // pad with spaces to ensure progress line overwritten
                sprintf(buf + p, "%*s", 80 - p, " ");
            x265_log(&param, X265_LOG_FULL, "%s\n", buf);
        }
    }
}
}
