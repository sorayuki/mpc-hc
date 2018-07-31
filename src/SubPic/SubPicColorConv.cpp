/*
 * (C) 2003-2006 SoraYuki
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"

#include "ISubPic.h"
#include "SubPicColorConv.h"
#include <assert.h>
#include <windows.h>
#include <mutex>

CSubPicColorConv* CSubPicColorConv::Get(CSubPicColorConv::Level level, CSubPicColorConv::ColorMatrix clrMat)
{
    static CSubPicColorConv* s_instances[LEVEL_COUNT][CM_COUNT];
    static std::mutex s_instanceMutex;

    assert(level >= 0 && level <= LEVEL_COUNT);
    assert(clrMat >= 0 && clrMat <= CM_COUNT);
    if (s_instances[level][clrMat] == 0)
    {
        std::lock_guard<std::mutex> lockGuard(s_instanceMutex);
        MemoryBarrier();
        if (s_instances[level][clrMat] == 0)
        {
            CSubPicColorConv* newInstance = new CSubPicColorConv(level, clrMat);
            MemoryBarrier();
            s_instances[level][clrMat] = newInstance;
        }
    }

    return s_instances[level][clrMat];
}

CSubPicColorConv* CSubPicColorConv::GetByRes(int w, int h)
{
    if (w > 1024 || h > 576)
        return Get(LEVEL_TV, CM_BT709);
    else
        return Get(LEVEL_TV, CM_BT601);
}

unsigned char CSubPicColorConv::RGB2Y(unsigned char r, unsigned char g, unsigned char b)
{
    if (m_level == LEVEL_TV)
        return clip[BYTE((c2y_yb[b] + c2y_yg[g] + c2y_yr[r] + 0x108000) >> 16)];
    else if (m_level == LEVEL_PC)
        return clip[BYTE((c2y_yb[b] + c2y_yg[g] + c2y_yr[r]) >> 16)];
    else
    {
        assert(false);
        return 0;
    }
}

unsigned char CSubPicColorConv::YB2U(int scaledY, unsigned char b)
{
    if (m_level == LEVEL_TV)
        return clip[((((b << 16) - scaledY) >> 10) * c2y_cu + 0x800000 + 0x8000) >> 16];
    else if (m_level == LEVEL_PC)
        return clip[((((b << 16) - scaledY) >> 10) * c2y_cu + 0x800000) >> 16];
    else
    {
        assert(false);
        return 0x80;
    }
}

unsigned char CSubPicColorConv::YR2V(int scaledY, unsigned char r)
{
    if (m_level == LEVEL_TV)
        return clip[((((r << 16) - scaledY) >> 10) * c2y_cv + 0x800000 + 0x8000) >> 16];
    else if (m_level == LEVEL_PC)
        return clip[((((r << 16) - scaledY) >> 10) * c2y_cv + 0x800000) >> 16];
    else
    {
        assert(false);
        return 0x80;
    }
}

int CSubPicColorConv::ScaleY(int y)
{
    if (m_level == LEVEL_PC)
        return y * cy_cy;
    else if (m_level == LEVEL_TV)
        return (y - 16) * cy_cy;
    else
    {
        assert(false);
        return y * cy_cy;
    }
}

CSubPicColorConv::CSubPicColorConv(int level, int clrMat)
    : clip(clipBase + 256)
    , m_level((CSubPicColorConv::Level)level)
    , m_clrMat((CSubPicColorConv::ColorMatrix)clrMat)
{
    static const float k_cyr[CM_COUNT] = { 0.299f, 0.2125f };
    static const float k_cyg[CM_COUNT] = { 0.587f, 0.7154f };
    static const float k_cyb[CM_COUNT] = { 0.114f, 0.0721f };

    static const float k_ylevel[LEVEL_COUNT] = { 1.0f, 219.f / 255.f };
    static const float k_uvlevel[LEVEL_COUNT] = { 1.0f, 224.f / 255.f };

    int c2y_cyb = std::lround(k_cyb[clrMat] * k_ylevel[level] * 65536);
    int c2y_cyg = std::lround(k_cyg[clrMat] * k_ylevel[level] * 65536);
    int c2y_cyr = std::lround(k_cyr[clrMat] * k_ylevel[level] * 65536);

    int y2c_cbu = std::lround((1 - k_cyb[clrMat]) * 2 / k_uvlevel[level] * 65536);
    int y2c_cgu = std::lround((1 - k_cyb[clrMat]) * 2 * k_cyb[clrMat] / k_uvlevel[level] * 65536);
    int y2c_cgv = std::lround((1 - k_cyr[clrMat]) * 2 * k_cyr[clrMat] / k_uvlevel[level] * 65536);
    int y2c_crv = std::lround((1 - k_cyr[clrMat]) * 2 / k_uvlevel[level] * 65536);

    c2y_cu = std::lround(1.0 / ((1 - k_cyb[clrMat]) * 2 / k_uvlevel[level]) * 1024);
    c2y_cv = std::lround(1.0 / ((1 - k_cyr[clrMat]) * 2 / k_uvlevel[level]) * 1024);

    cy_cy = std::lround(1 / k_ylevel[level] * 65536);

    for (int i = 0; i < 256; i++) {
        clipBase[i] = 0;
        clipBase[i + 256] = BYTE(i);
        clipBase[i + 512] = 255;
    }

    for (int i = 0; i < 256; i++) {
        c2y_yb[i] = c2y_cyb * i;
        c2y_yg[i] = c2y_cyg * i;
        c2y_yr[i] = c2y_cyr * i;

        y2c_bu[i] = y2c_cbu * (i - 128);
        y2c_gu[i] = y2c_cgu * (i - 128);
        y2c_gv[i] = y2c_cgv * (i - 128);
        y2c_rv[i] = y2c_crv * (i - 128);
    }
}

// original code, for reference
#if 0
static unsigned char clipBase[256 * 3];
static unsigned char* clip = clipBase + 256;

static const int c2y_cyb = std::lround(0.114 * 219 / 255 * 65536);
static const int c2y_cyg = std::lround(0.587 * 219 / 255 * 65536);
static const int c2y_cyr = std::lround(0.299 * 219 / 255 * 65536);
static const int c2y_cu = std::lround(1.0 / 2.018 * 1024);
static const int c2y_cv = std::lround(1.0 / 1.596 * 1024);

int c2y_yb[256];
int c2y_yg[256];
int c2y_yr[256];

static const int y2c_cbu = std::lround(2.018 * 65536);
static const int y2c_cgu = std::lround(0.391 * 65536);
static const int y2c_cgv = std::lround(0.813 * 65536);
static const int y2c_crv = std::lround(1.596 * 65536);
static int y2c_bu[256];
static int y2c_gu[256];
static int y2c_gv[256];
static int y2c_rv[256];

static const int cy_cy = std::lround(255.0 / 219.0 * 65536);
static const int cy_cy2 = std::lround(255.0 / 219.0 * 32768);

void ColorConvInit()
{
    static bool bColorConvInitOK = false;
    if (bColorConvInitOK) {
        return;
    }

    for (int i = 0; i < 256; i++) {
        clipBase[i] = 0;
        clipBase[i + 256] = BYTE(i);
        clipBase[i + 512] = 255;
    }

    for (int i = 0; i < 256; i++) {
        c2y_yb[i] = c2y_cyb * i;
        c2y_yg[i] = c2y_cyg * i;
        c2y_yr[i] = c2y_cyr * i;

        y2c_bu[i] = y2c_cbu * (i - 128);
        y2c_gu[i] = y2c_cgu * (i - 128);
        y2c_gv[i] = y2c_cgv * (i - 128);
        y2c_rv[i] = y2c_crv * (i - 128);
    }

    bColorConvInitOK = true;
}
#endif
