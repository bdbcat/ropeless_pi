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
}

bool RFA::Parse( const SENTENCE& sentence )
{


   /*
   ** RFA - Ropeless Fishing Messsage A
   **
   **  Version Draft "C" Format
   **
   **        1        2       3       4     5   6    7    8      9      10      11      12    13   14
   **        |        |       |       |     |   |    |    |      |      |       |       |     |    |
   ** $--RFA,xxxm.yyy,llll.ll,llll.ll,xxx.x,ccc,cccc,cccc,xxxx.x,xxx.xx,llll.ll,llll.ll,xxx.x,xx.x,xx*hh<CR><LF>
   **
   ** Field Number:
   **  1) Timestamp
   **  2) Vessel Latitude
   **  3) Vessel Longitude
   **  4) Vessel Heading
   **  5) Mfg Code
   **  6) Transponder Code
   **  7) Transponder Code, trawl partner
   **  8) Range In meters to Transponder
   **  9) Bearing to Transponder
   ** 10) Predicted Transponder location Lat
   ** 11) Predicted Transponder location Lon
   ** 12) Depth
   ** 13) Temperature
   ** 14) Battery Status
   ** 11) Checksum
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

   if (nFields > 11) {
      TimeStamp = sentence.Double(1);
      OwnshipPosition.Parse(2, 3, sentence);
      OwnshipHeading = sentence.Double(4);
      TransponderMFG = sentence.Field(5);
      TransponderCode = sentence.Integer(6);
      TransponderPartner = sentence.Integer(7);
      TransponderRange = sentence.Double(8);
      TransponderBearing = sentence.Double(9);

      TransponderPosition.Parse(10, 11, sentence);
      TransponderDepth = sentence.Double(12);
      TransponderTemp = sentence.Double(13);
      TransponderBattStat = sentence.Integer(14);
   } else {
      TimeStamp = sentence.Double(1);
      OwnshipPosition.Parse(2, 3, sentence);
      OwnshipHeading = sentence.Double(4);
      TransponderCode = sentence.Integer(5);
      TransponderPartner = sentence.Integer(6);
      TransponderRange = sentence.Double(7);
      TransponderBearing = sentence.Double(8);

      TransponderPosition.Parse(9, 10, sentence);
   }


   return( TRUE );
}

bool RFA::Write( SENTENCE& sentence )
{
//   ASSERT_VALID( this );

   /*
   ** Let the parent do its thing
   */

   RESPONSE::Write( sentence );
   return( TRUE );
}

const RFA& RFA::operator = ( const RFA& source )
{
  return( *this );
}
