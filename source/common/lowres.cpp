/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@multicorewareinc.com>
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

using namespace x265;

bool Lowres::create(TComPicYuv *orig, int _bframes, bool bAQEnabled)
{
    isLowres = true;
    bframes = _bframes;
    width = orig->getWidth() / 2;
    lines = orig->getHeight() / 2;
    lumaStride = width + 2 * orig->getLumaMarginX();
    if (lumaStride & 31)
        lumaStride += 32 - (lumaStride & 31);
    int cuWidth = (width + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    int cuHeight = (lines + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    int cuCount = cuWidth * cuHeight;

    /* rounding the width to multiple of lowres CU size */
    width = cuWidth * X265_LOWRES_CU_SIZE;
    lines = cuHeight * X265_LOWRES_CU_SIZE;

    size_t planesize = lumaStride * (lines + 2 * orig->getLumaMarginY());
    size_t padoffset = lumaStride * orig->getLumaMarginY() + orig->getLumaMarginX();

    if (bAQEnabled)
    {
        CHECKED_MALLOC(qpAqOffset, double, cuCount);
        CHECKED_MALLOC(invQscaleFactor, int, cuCount);
        CHECKED_MALLOC(qpCuTreeOffset, double, cuCount);
    }
    CHECKED_MALLOC(propagateCost, uint16_t, cuCount);

    /* allocate lowres buffers */
    for (int i = 0; i < 4; i++)
    {
        CHECKED_MALLOC(buffer[i], pixel, planesize);
        /* initialize the whole buffer to prevent valgrind warnings on right edge */
        memset(buffer[i], 0, sizeof(pixel) * planesize);
    }

    lowresPlane[0] = buffer[0] + padoffset;
    lowresPlane[1] = buffer[1] + padoffset;
    lowresPlane[2] = buffer[2] + padoffset;
    lowresPlane[3] = buffer[3] + padoffset;

    CHECKED_MALLOC(intraCost, int32_t, cuCount);

    for (int i = 0; i < bframes + 2; i++)
    {
        for (int j = 0; j < bframes + 2; j++)
        {
            CHECKED_MALLOC(rowSatds[i][j], int32_t, cuHeight);
            CHECKED_MALLOC(lowresCosts[i][j], uint16_t, cuCount);
        }
    }

    for (int i = 0; i < bframes + 1; i++)
    {
        CHECKED_MALLOC(lowresMvs[0][i], MV, cuCount);
        CHECKED_MALLOC(lowresMvs[1][i], MV, cuCount);
        CHECKED_MALLOC(lowresMvCosts[0][i], int32_t, cuCount);
        CHECKED_MALLOC(lowresMvCosts[1][i], int32_t, cuCount);
    }

    return true;

fail:
    return false;
}

void Lowres::destroy()
{
    for (int i = 0; i < 4; i++)
    {
        X265_FREE(buffer[i]);
    }

    X265_FREE(intraCost);

    for (int i = 0; i < bframes + 2; i++)
    {
        for (int j = 0; j < bframes + 2; j++)
        {
            X265_FREE(rowSatds[i][j]);
            X265_FREE(lowresCosts[i][j]);
        }
    }

    for (int i = 0; i < bframes + 1; i++)
    {
        X265_FREE(lowresMvs[0][i]);
        X265_FREE(lowresMvs[1][i]);
        X265_FREE(lowresMvCosts[0][i]);
        X265_FREE(lowresMvCosts[1][i]);
    }

    X265_FREE(qpAqOffset);
    X265_FREE(invQscaleFactor);
    X265_FREE(qpCuTreeOffset);
    X265_FREE(propagateCost);
}

// (re) initialize lowres state
void Lowres::init(TComPicYuv *orig, int poc, int type)
{
    bIntraCalculated = false;
    bLastMiniGopBFrame = false;
    bScenecut = true;  // could be a scene-cut, until ruled out by flash detection
    bKeyframe = false; // Not a keyframe unless identified by lookahead
    sliceType = type;
    frameNum = poc;
    leadingBframes = 0;
    satdCost = (int64_t)-1;
    memset(costEst, -1, sizeof(costEst));
    memset(weightedCostDelta, 0, sizeof(weightedCostDelta));

    if (qpAqOffset && invQscaleFactor)
        memset(costEstAq, -1, sizeof(costEstAq));

    for (int y = 0; y < bframes + 2; y++)
    {
        for (int x = 0; x < bframes + 2; x++)
        {
            rowSatds[y][x][0] = -1;
        }
    }

    for (int i = 0; i < bframes + 1; i++)
    {
        lowresMvs[0][i][0].x = 0x7FFF;
        lowresMvs[1][i][0].x = 0x7FFF;
    }

    for (int i = 0; i < bframes + 2; i++)
    {
        intraMbs[i] = 0;
    }

    /* downscale and generate 4 hpel planes for lookahead */
    primitives.frame_init_lowres_core(orig->getLumaAddr(),
                                      lowresPlane[0], lowresPlane[1], lowresPlane[2], lowresPlane[3],
                                      orig->getStride(), lumaStride, width, lines);

    /* extend hpel planes for motion search */
    extendPicBorder(lowresPlane[0], lumaStride, width, lines, orig->getLumaMarginX(), orig->getLumaMarginY());
    extendPicBorder(lowresPlane[1], lumaStride, width, lines, orig->getLumaMarginX(), orig->getLumaMarginY());
    extendPicBorder(lowresPlane[2], lumaStride, width, lines, orig->getLumaMarginX(), orig->getLumaMarginY());
    extendPicBorder(lowresPlane[3], lumaStride, width, lines, orig->getLumaMarginX(), orig->getLumaMarginY());
    fpelPlane = lowresPlane[0];
}
