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



RFA::RFA()
{
    Mnemonic = _T("RFA");
   Empty();
}

RFA::~RFA()
{
   Mnemonic.Empty();
   Empty();
}

void RFA::Empty( void )
{
#if 0
   UTCTime.Empty();
   IsDataValid                = Unknown0183;
   SpeedOverGroundKnots       = 0.0;
   Position.Empty();
   TrackMadeGoodDegreesTrue   = 0.0;
   Date.Empty();
   MagneticVariation          = 0.0;
   MagneticVariationDirection = EW_Unknown;
#endif
}

bool RFA::Parse( const SENTENCE& sentence )
{
//   ASSERT_VALID( this );

/*
1. Loop Iteration (Ignore)
 2. Transponder Code. Here we have codes 1 2 3 4
 3. Range In meters.
 4. Bearing to Transponder
5. Detection Index. (How well the signal correlates. Ignore)
 6. Vessel Heading
 7. Vessel Lat.
 8. Vessel Lon
 9. Date Number
10. Range in degrees lat.
 11. Predicted Transponder location Lat.
 12. Predicted Transponder location Lon.
*/

   /*
   ** RFA - Ropeless Fishing Messsage A
   **
   **  Version 1.0 Format
   **
   **        1        2       3       4   5    6    7    8       9      10      11      12
   **        |        |       |       |   |    |    |    |       |      |       |       |
   ** $--RFA,xxxm.yyy,llll.ll,llll.ll,xxx,cccc,cccc,cccc,xxxx.xx,xxx.xx,llll.ll,llll.ll*hh<CR><LF>
   **
   ** Field Number:
   **  1) Timestamp
   **  2) Vessel Latitude
   **  3) Vessel Longitude
   **  4) Vessel Heading
   **  5) Transponder Code
   **  6) Transponder Code, trawl partner
   **  7) Range In meters to Transponder
   **  8) Bearing to Transponder
   **  9) Predicted Transponder location Lat
   ** 10) Predicted Transponder location Lon
   ** 11) Checksum
   */

#if 0
   Followed the bellow format.
% $--RFA,xxxm.yyy,llll.ll,llll.ll,xxx,cccc,cccc,cccc,xxxx.xx,xxx.xx,llll.ll,llll.ll*hh<CR><LF>
%  Field Number:
%  1) Timestamp
%  2) Vessel Latitude
%  3) Vessel Longitude
%  4) Vessel Heading
%  5) Transponder Code
%  6) Transponder Trawl Pair Code
%  7) Range In meters to Transponder
%  8) Bearing to Transponder
%  9) Predicted Transponder location Lat
%  10) Predicted Transponder location Lon
%  11) Checksum

#endif
   /*
   ** First we check the checksum...
   */

   int nFields = sentence.GetNumberOfDataFields( );

   NMEA0183_BOOLEAN check = sentence.IsChecksumBad( nFields + 1 );

   if ( check == NTrue )
   {
       return( FALSE );
   }

   OwnshipPosition.Parse(2, 3, sentence);
   OwnshipHeading = sentence.Double(4);
   TimeStamp = sentence.Double(1);

   TransponderPosition.Parse(9, 10, sentence);
   TransponderCode = sentence.Integer(5);
   TransponderPartner = sentence.Integer(6);
   TransponderRange = sentence.Double(7);
   TransponderBearing = sentence.Double(8);


   return( TRUE );
}

bool RFA::Write( SENTENCE& sentence )
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

const RFA& RFA::operator = ( const RFA& source )
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
