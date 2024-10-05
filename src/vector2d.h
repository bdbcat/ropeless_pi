/******************************************************************************
 *
 * Project:  OpenCPN PlugIn support
 *
 ***************************************************************************
 *   Copyright (C) 2014 by David S. Register                               *
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
#ifndef _VECTOR2D_H_
#define _VECTOR2D_H_

class vector2D
{
public:
    vector2D() { x = 0.0; y = 0.0; }
    vector2D( double a, double b ) { x = a; y = b; }
    friend bool operator==( vector2D &a, vector2D &b ) { return a.x == b.x && a.y == b.y; }
    friend bool operator!=( vector2D &a, vector2D &b ) { return a.x != b.x || a.y != b.y; }
    friend vector2D operator-( vector2D a, vector2D b ) { return vector2D( a.x - b.x, a.y - b.y ); }
    friend vector2D operator+( vector2D a, vector2D b ) { return vector2D( a.x + b.x, a.y + b.y ); }
    friend vector2D operator*( double t, vector2D a ) { return vector2D( a.x * t, a.y * t ); }

    union{ double x; double lon; };
    union{ double y; double lat; };
};

typedef vector2D* pVector2D;

extern "C" double vGetLengthOfNormal( pVector2D a, pVector2D b, pVector2D n );
extern "C" double vDotProduct(pVector2D v0, pVector2D v1);
extern "C" pVector2D vAddVectors(pVector2D v0, pVector2D v1, pVector2D v);
extern "C" pVector2D vSubtractVectors(pVector2D v0, pVector2D v1, pVector2D v);
extern "C" double vVectorMagnitude(pVector2D v0);
extern "C" double vVectorSquared(pVector2D v0);

typedef struct  {
    double y;
    double x;
} MyFlPoint;

extern "C" bool Intersect(MyFlPoint p1, MyFlPoint p2, MyFlPoint p3, MyFlPoint p4);
extern "C" bool LineIntersect(MyFlPoint p1, MyFlPoint p2, MyFlPoint p3, MyFlPoint p4, double *int_x, double *int_y);
extern "C" bool G_FloatPtInPolygon(MyFlPoint *rgpts, int wnumpts, float x, float y);

#endif
