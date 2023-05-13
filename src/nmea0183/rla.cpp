/***************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  NMEA0183 Support Classes
 * Author:   Samuel R. Blackburn, David S. Register
 *
 ***************************************************************************
 *   Copyright (C) 2010 by Samuel R. Blackburn, David S Register           *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 *
 *   S Blackburn's original source license:                                *
 *         "You can use it any way you like."                              *
 *   More recent (2010) license statement:                                 *
 *         "It is BSD license, do with it what you will"                   *
 */


#include "nmea0183.h"



RLA::RLA()
{
    Mnemonic = _T("RLA");
   Empty();
}

RLA::~RLA()
{
   Mnemonic.Empty();
   Empty();
}

void RLA::Empty( void )
{
}

bool RLA::Parse( const SENTENCE& sentence )
{
//   ASSERT_VALID( this );


   /*
   ** RLA - Ropeless Fishing Release Message A
   **
   **  Version 1.0 Format
   **
   **        1    2    3
   **        |    |    |
   ** $--RLA,cccc,ssss*hh<CR><LF>
   **
   ** Field Number:
   **  1) Transponder Code
   **  2) TransponderStatus
   **  3) Checksum
   */

   /*
   ** First we check the checksum...
   */

   int nFields = sentence.GetNumberOfDataFields( );

   NMEA0183_BOOLEAN check = sentence.IsChecksumBad( nFields + 1 );

   if ( check == NTrue )
   {
       return( FALSE );
   }

   TransponderCode = sentence.Integer(1);
   TransponderStatus = sentence.Integer(2);


   return( TRUE );
}

bool RLA::Write( SENTENCE& sentence )
{
//   ASSERT_VALID( this );

   /*
   ** Let the parent do its thing
   */

   RESPONSE::Write( sentence );
#if 0
   sentence += UTCTime;
   sentence += IsDataValid;
   sentence += Position;
   sentence += SpeedOverGroundKnots;
   sentence += TrackMadeGoodDegreesTrue;
   sentence += Date;
   sentence += MagneticVariation;
   sentence += MagneticVariationDirection;

   sentence.Finish();
#endif
   return( TRUE );
}

const RLA& RLA::operator = ( const RLA& source )
{
//   ASSERT_VALID( this );
#if 0
   UTCTime                    = source.UTCTime;
   IsDataValid                = source.IsDataValid;
   Position                   = source.Position;
   SpeedOverGroundKnots       = source.SpeedOverGroundKnots;
   TrackMadeGoodDegreesTrue   = source.TrackMadeGoodDegreesTrue;
   Date                       = source.Date;
   MagneticVariation          = source.MagneticVariation;
   MagneticVariationDirection = source.MagneticVariationDirection;
#endif

  return( *this );
}
