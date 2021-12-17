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

#include "vector2d.h"
#include <math.h>
#include <stddef.h>

//---------------------------------------------------------------------------------
//          Vector Stuff for Hit Test Algorithm
//---------------------------------------------------------------------------------
double vGetLengthOfNormal( pVector2D a, pVector2D b, pVector2D n )
{
    vector2D c, vNormal;
    vNormal.x = 0;
    vNormal.y = 0;
    //
    //Obtain projection vector.
    //
    //c = ((a * b)/(|b|^2))*b
    //
    c.x = b->x * ( vDotProduct( a, b ) / vDotProduct( b, b ) );
    c.y = b->y * ( vDotProduct( a, b ) / vDotProduct( b, b ) );
//
    //Obtain perpendicular projection : e = a - c
    //
    vSubtractVectors( a, &c, &vNormal );
    //
    //Fill PROJECTION structure with appropriate values.
    //
    *n = vNormal;

    return ( vVectorMagnitude( &vNormal ) );
}

double vDotProduct( pVector2D v0, pVector2D v1 )
{
    double dotprod;

    dotprod = ( v0 == NULL || v1 == NULL ) ? 0.0 : ( v0->x * v1->x ) + ( v0->y * v1->y );

    return ( dotprod );
}

pVector2D vAddVectors( pVector2D v0, pVector2D v1, pVector2D v )
{
    if( v0 == NULL || v1 == NULL ) v = (pVector2D) NULL;
    else {
        v->x = v0->x + v1->x;
        v->y = v0->y + v1->y;
    }
    return ( v );
}

pVector2D vSubtractVectors( pVector2D v0, pVector2D v1, pVector2D v )
{
    if( v0 == NULL || v1 == NULL ) v = (pVector2D) NULL;
    else {
        v->x = v0->x - v1->x;
        v->y = v0->y - v1->y;
    }
    return ( v );
}

double vVectorSquared( pVector2D v0 )
{
    double dS;

    if( v0 == NULL ) dS = 0.0;
    else
        dS = ( ( v0->x * v0->x ) + ( v0->y * v0->y ) );
    return ( dS );
}

double vVectorMagnitude( pVector2D v0 )
{
    double dMagnitude;

    if( v0 == NULL ) dMagnitude = 0.0;
    else
        dMagnitude = sqrt( vVectorSquared( v0 ) );
    return ( dMagnitude );
}

/*************************************************************************
 * 
 * 
 * FUNCTION:   CCW (CounterClockWise)
 *
 * PURPOSE
 * Determines, given three points, if when travelling from the first to
 * the second to the third, we travel in a counterclockwise direction.
 *
 * RETURN VALUE
 * (int) 1 if the movement is in a counterclockwise direction, -1 if
 * not.
 *************************************************************************/


int CCW(MyFlPoint p0, MyFlPoint p1, MyFlPoint p2) {
    float dx1, dx2 ;
    float dy1, dy2 ;
    
    dx1 = p1.x - p0.x ; dx2 = p2.x - p0.x ;
    dy1 = p1.y - p0.y ; dy2 = p2.y - p0.y ;
    
    /* This is basically a slope comparison: we don't do divisions because
     * 
     * of divide by zero possibilities with pure horizontal and pure
     * vertical lines.
     */
    return ((dx1 * dy2 > dy1 * dx2) ? 1 : -1) ;
    
}
/*************************************************************************
 * 
 * 
 * FUNCTION:   Intersect
 *
 * PURPOSE
 * Given two line segments, determine if they intersect.
 *
 * RETURN VALUE
 * TRUE if they intersect, FALSE if not.
 *************************************************************************/


bool Intersect(MyFlPoint p1, MyFlPoint p2, MyFlPoint p3, MyFlPoint p4) {
    return ((( CCW(p1, p2, p3) * CCW(p1, p2, p4)) <= 0)
    && (( CCW(p3, p4, p1) * CCW(p3, p4, p2)  <= 0) )) ;
    
}

bool LineIntersect(MyFlPoint p1, MyFlPoint p2, MyFlPoint p3, MyFlPoint p4, double *int_x, double *int_y)
{
    if(!Intersect( p1, p2, p3, p4))
        return false;

    //http://www-cs.ccny.cuny.edu/~wolberg/capstone/intersection/Intersection%20point%20of%20two%20lines.html
    double u = ((p4.x - p3.x) * (p1.y - p3.y)) - ((p4.y - p3.y) * (p1.x - p3.x));
    u /= ((p4.y - p3.y) * (p2.x - p1.x)) - ((p4.x - p3.x) * (p2.y - p1.y));
    
    if(int_x)
        *int_x = p1.x + u * (p2.x - p1.x);
    if(int_y)
        *int_y = p1.y + u * (p2.y -p1.y);    

    return true;
    
}

/*************************************************************************
 * 
 * 
 * FUNCTION:   G_FloatPtInPolygon
 *
 * PURPOSE
 * This routine determines if the point passed is in the polygon. It uses
 * 
 * the classical polygon hit-testing algorithm: a horizontal ray starting
 * 
 * at the point is extended infinitely rightwards and the number of
 * polygon edges that intersect the ray are counted. If the number is odd,
 * the point is inside the polygon.
 *
 * RETURN VALUE
 * (bool) TRUE if the point is inside the polygon, FALSE if not.
 *************************************************************************/


bool G_FloatPtInPolygon(MyFlPoint *rgpts, int wnumpts, float x, float y)

{
    
    MyFlPoint  *ppt, *ppt1 ;
    int   i ;
    MyFlPoint  pt1, pt2, pt0 ;
    int   wnumintsct = 0 ;
    
    pt0.x = x;
    pt0.y = y;
    
    pt1 = pt2 = pt0 ;
    pt2.x = 1.e6;
    
    // Now go through each of the lines in the polygon and see if it
    // intersects
    for (i = 0, ppt = rgpts ; i < wnumpts-1 ; i++, ppt++)
    {
        ppt1 = ppt;
        ppt1++;
        if (Intersect(pt0, pt2, *ppt, *(ppt1)))
            wnumintsct++ ;
    }
    
    // And the last line
    if (Intersect(pt0, pt2, *ppt, *rgpts))
        wnumintsct++ ;
    
    //   return(wnumintsct&1);
        
        //       If result is false, check the degenerate case where test point lies on a polygon endpoint
        if(!(wnumintsct&1))
        {
            for (i = 0, ppt = rgpts ; i < wnumpts ; i++, ppt++)
            {
                if(((*ppt).x == x) && ((*ppt).y == y))
                    return true;
            }
        }
        else
            return true;
        
        return false;
}



