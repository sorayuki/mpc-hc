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

#ifndef SUBPICCOLORCONV_H_
#define SUBPICCOLORCONV_H_
 
class CSubPicColorConv
{
public:
    enum Level { LEVEL_PC = 0, LEVEL_TV, LEVEL_COUNT };
    enum ColorMatrix { CM_BT601, CM_BT709, CM_COUNT };

    static CSubPicColorConv* Get(Level level, ColorMatrix clrMat);
    static CSubPicColorConv* GetByRes(int w, int h);

    unsigned char clipBase[256 * 3];
    unsigned char* clip;

    int c2y_yb[256];
    int c2y_yg[256];
    int c2y_yr[256];

    int y2c_bu[256];
    int y2c_gu[256];
    int y2c_gv[256];
    int y2c_rv[256];

    int c2y_cu;
    int c2y_cv;

    int cy_cy;
    int cy_cy2;

    int ScaleY(int y);

    unsigned char RGB2Y(unsigned char r, unsigned char g, unsigned char b);
    unsigned char YB2U(int scaledY, unsigned char b);
    unsigned char YR2V(int scaledY, unsigned char r);

private:
    Level m_level;
    ColorMatrix m_clrMat;

    CSubPicColorConv(int level, int clrMat);
};

#endif