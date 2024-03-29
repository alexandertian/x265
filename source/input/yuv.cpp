/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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

#include "yuv.h"
#include "PPA/ppa.h"
#include "common.h"

#include <iostream>

#define ENABLE_THREADING 1

#if _WIN32
#include <io.h>
#include <fcntl.h>
#if defined(_MSC_VER)
#pragma warning(disable: 4996) // POSIX setmode and fileno deprecated
#endif
#endif

using namespace x265;
using namespace std;

YUVInput::YUVInput(InputFileInfo& info)
{
    for (int i = 0; i < QUEUE_SIZE; i++)
    {
        buf[i] = NULL;
        frameStat[i] = false;
    }

    head.set(0);
    tail.set(0);
    framesize = 0;
    depth = info.depth;
    width = info.width;
    height = info.height;
    colorSpace = info.csp;
    threadActive = false;
    ifs = NULL;

    if (width == 0 || height == 0 || info.fpsNum == 0 || info.fpsDenom == 0)
    {
        x265_log(NULL, X265_LOG_ERROR, "yuv: width, height, and FPS must be specified\n");
        return;
    }

    if (!strcmp(info.filename, "-"))
    {
        ifs = &cin;
#if _WIN32
        setmode(fileno(stdin), O_BINARY);
#endif
    }
    else
        ifs = new ifstream(info.filename, ios::binary | ios::in);

    if (ifs && ifs->good())
        threadActive = true;
    else
    {
        if (ifs && ifs != &cin)
            delete ifs;
        ifs = NULL;
        return;
    }

    pixelbytes = depth > 8 ? 2 : 1;
    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        uint32_t w = width >> x265_cli_csps[colorSpace].width[i];
        uint32_t h = height >> x265_cli_csps[colorSpace].height[i];
        framesize += w * h * pixelbytes;
    }

    for (uint32_t i = 0; i < QUEUE_SIZE; i++)
    {
        buf[i] = new char[framesize];
        if (buf[i] == NULL)
        {
            x265_log(NULL, X265_LOG_ERROR, "yuv: buffer allocation failure, aborting\n");
            threadActive = false;
            return;
        }
    }

    info.frameCount = -1;

    /* try to estimate frame count, if this is not stdin */
    if (ifs != &cin)
    {
        istream::pos_type cur = ifs->tellg();

#if defined(_MSC_VER) && _MSC_VER < 1700
        /* Older MSVC versions cannot handle 64bit file sizes properly, so go native */
        HANDLE hFile = CreateFileA(info.filename, GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER size;
            if (GetFileSizeEx(hFile, &size))
                info.frameCount = (int)((size.QuadPart - (int64_t)cur) / framesize);
            CloseHandle(hFile);
        }
#else // if defined(_MSC_VER) && _MSC_VER < 1700
        if (cur >= 0)
        {
            ifs->seekg(0, ios::end);
            istream::pos_type size = ifs->tellg();
            ifs->seekg(cur, ios::beg);
            if (size > 0)
                info.frameCount = (int)((size - cur) / framesize);
        }
#endif // if defined(_MSC_VER) && _MSC_VER < 1700
    }

    if (info.skipFrames)
    {
#if X86_64
        ifs->seekg((uint64_t)framesize * info.skipFrames, ios::cur);
#else
        for (int i = 0; i < info.skipFrames; i++)
        {
            ifs->ignore(framesize);
        }
#endif
    }
}

YUVInput::~YUVInput()
{
    if (ifs && ifs != &cin)
        delete ifs;
    for (int i = 0; i < QUEUE_SIZE; i++)
    {
        delete[] buf[i];
    }
}

void YUVInput::release()
{
    threadActive = false;
    head.set(QUEUE_SIZE);
    stop();
    delete this;
}

void YUVInput::startReader()
{
#if ENABLE_THREADING
    if (ifs && threadActive)
        start();
#endif
}

void YUVInput::threadMain()
{
    while (threadActive)
    {
        if (!populateFrameQueue())
            break;
    }

    threadActive = false;
    frameStat[tail.get()] = false;
    tail.set(QUEUE_SIZE);
}

bool YUVInput::populateFrameQueue()
{
    int curTail = tail.get();
    int curHead = head.get();

    while ((curTail + 1) % QUEUE_SIZE == curHead)
    {
        curHead = head.waitForChange(curHead);
        if (!threadActive)
            return false;
    }

    PPAStartCpuEventFunc(read_yuv);
    ifs->read(buf[curTail], framesize);
    frameStat[curTail] = !ifs->fail();
    tail.set((curTail + 1) % QUEUE_SIZE);
    PPAStopCpuEventFunc(read_yuv);

    return !ifs->fail();
}

bool YUVInput::readPicture(x265_picture& pic)
{
    int curHead = head.get();
    int curTail = tail.get();

#if ENABLE_THREADING

    while (curHead == curTail)
    {
        curTail = tail.waitForChange(curTail);
        if (!threadActive)
            return false;
    }

#else

    populateFrameQueue();

#endif // if ENABLE_THREADING

    if (!frameStat[curHead])
        return false;

    pic.colorSpace = colorSpace;
    pic.bitDepth = depth;
    pic.stride[0] = width * pixelbytes;
    pic.stride[1] = pic.stride[0] >> x265_cli_csps[colorSpace].width[1];
    pic.stride[2] = pic.stride[0] >> x265_cli_csps[colorSpace].width[2];
    pic.planes[0] = buf[curHead];
    pic.planes[1] = (char*)pic.planes[0] + pic.stride[0] * height;
    pic.planes[2] = (char*)pic.planes[1] + pic.stride[1] * (height >> x265_cli_csps[colorSpace].height[1]);

    head.set((curHead + 1) % QUEUE_SIZE);

    return true;
}
