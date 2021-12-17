/******************************************************************************
 *
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 */

#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include "TexFont.h"


void AlphaBlending( wxDC *pdc, int x, int y, int size_x, int size_y, float radius, wxColour color,
                    unsigned char transparency );
void AlphaBlendingPoly( wxDC *pdc, int n, wxPoint points[], wxColour color, unsigned char transparency );

void RenderLine(wxDC *dc, void *pgc, int x1, int y1, int x2, int y2, wxColour color, int width);
void RenderText( wxDC *dc, void *pgc, wxString &msg, wxFont *font, wxColour &color, int xp, int yp, double angle);

unsigned int GetIconTexture( wxBitmap *pbm, int &glw, int &glh );
void renderTexture( unsigned int IconTexture, wxPoint r, int glw, int glh);


//void DrawLine(int x1, int y1, int x2, int y2);

//void GLDrawLine( wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2 );
void RenderGLText( TexFont *ptf, wxString &msg, wxFont *font, int xp, int yp, double angle);


#endif

