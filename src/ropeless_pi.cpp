/***************************************************************************
 *
 * Project:  OpenCPN
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

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers
#include <typeinfo>
#include <wx/graphics.h>

#include "ropeless_pi.h"
#include "icons.h"
#include "Select.h"
#include "vector2d.h"
#include "dsPortType.h"
#include "OCP_DataStreamInput_Thread.h"
#include "OCPN_DataStreamEvent.h"
#include "georef.h"

#ifdef ocpnUSE_GL
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double*)&lNaN)
#endif

//wxEventType wxEVT_PI_OCPN_DATASTREAM;// = wxNewEventType();

#ifdef __WXMSW__
wxEventType wxEVT_PI_OCPN_DATASTREAM = wxNewEventType();
#else
wxEventType wxEVT_PI_OCPN_DATASTREAM;// = wxNewEventType();
#endif

//      Global Static variable
PlugIn_ViewPort         *g_vp;
PlugIn_ViewPort         g_ovp;

double                   g_Var;         // assummed or calculated variation

NMEA0183             g_NMEA0183;                 // Used to parse NMEA Sentences
double gLat, gLon, gSog, gCog, gHdt, gHdm, gVar;
bool                    ll_valid;
bool                    pos_valid;

#if wxUSE_GRAPHICS_CONTEXT
wxGraphicsContext       *g_gdc;
#endif
wxDC                    *g_pdc;

wxArrayString           g_iconTypeArray;
int                     gHDT_Watchdog;
int                     gGPS_Watchdog;

static int s_ownship_icon[] = { 5, -42, 11, -28, 11, 42, -11, 42, -11, -28, -5, -42, -11, 0, 11, 0,
0, 42, 0, -42
};

#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!
WX_DEFINE_OBJARRAY(ArrayOfBrgLines);
WX_DEFINE_OBJARRAY(ArrayOf2DPoints);

#include "default_pi.xpm"

wxString colorTableNames[] = {
  "MAGENTA",
  "CYAN",
  "LIME GREEN",
  "ORANGE",
  "MEDIUM GOLDENROD"
};
#define COLOR_TABLE_COUNT 5

wxString msgFileName = "/home/dsr/Projects/ropeless_pi/testset1.txt";

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi( void *ppimgr )
{
    return (opencpn_plugin *) new ropeless_pi( ppimgr );
}

extern "C" DECL_EXP void destroy_pi( opencpn_plugin* p )
{
    delete p;
}

wxString getWaypointName( wxString &GUID )
{
    PlugIn_Waypoint pwp;
    if(GetSingleWaypoint( GUID, &pwp ))
        return pwp.m_MarkName;
    else
        return _T("");
}


/*  These two function were taken from gpxdocument.cpp */
int GetRandomNumber(int range_min, int range_max)
{
      long u = (long)wxRound(((double)rand() / ((double)(RAND_MAX) + 1) * (range_max - range_min)) + range_min);
      return (int)u;
}

// RFC4122 version 4 compliant random UUIDs generator.
wxString GetUUID(void)
{
      wxString str;
      struct {
      int time_low;
      int time_mid;
      int time_hi_and_version;
      int clock_seq_hi_and_rsv;
      int clock_seq_low;
      int node_hi;
      int node_low;
      } uuid;

      uuid.time_low = GetRandomNumber(0, 2147483647);//FIXME: the max should be set to something like MAXINT32, but it doesn't compile un gcc...
      uuid.time_mid = GetRandomNumber(0, 65535);
      uuid.time_hi_and_version = GetRandomNumber(0, 65535);
      uuid.clock_seq_hi_and_rsv = GetRandomNumber(0, 255);
      uuid.clock_seq_low = GetRandomNumber(0, 255);
      uuid.node_hi = GetRandomNumber(0, 65535);
      uuid.node_low = GetRandomNumber(0, 2147483647);

      /* Set the two most significant bits (bits 6 and 7) of the
      * clock_seq_hi_and_rsv to zero and one, respectively. */
      uuid.clock_seq_hi_and_rsv = (uuid.clock_seq_hi_and_rsv & 0x3F) | 0x80;

      /* Set the four most significant bits (bits 12 through 15) of the
      * time_hi_and_version field to 4 */
      uuid.time_hi_and_version = (uuid.time_hi_and_version & 0x0fff) | 0x4000;

      str.Printf(_T("%08x-%04x-%04x-%02x%02x-%04x%08x"),
      uuid.time_low,
      uuid.time_mid,
      uuid.time_hi_and_version,
      uuid.clock_seq_hi_and_rsv,
      uuid.clock_seq_low,
      uuid.node_hi,
      uuid.node_low);

      return str;
}

void Clone_VP(PlugIn_ViewPort *dest, PlugIn_ViewPort *src)
{
    dest->clat =                   src->clat;                   // center point
    dest->clon =                   src->clon;
    dest->view_scale_ppm =         src->view_scale_ppm;
    dest->skew =                   src->skew;
    dest->rotation =               src->rotation;
    dest->chart_scale =            src->chart_scale;
    dest->pix_width =              src->pix_width;
    dest->pix_height =             src->pix_height;
    dest->rv_rect =                src->rv_rect;
    dest->b_quilt =                src->b_quilt;
    dest->m_projection_type =      src->m_projection_type;

    dest->lat_min =                src->lat_min;
    dest->lat_max =                src->lat_max;
    dest->lon_min =                src->lon_min;
    dest->lon_max =                src->lon_max;

    dest->bValid =                 src->bValid;                 // This VP is valid
}
//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

//      Event Handler implementation
BEGIN_EVENT_TABLE ( ropeless_pi, wxEvtHandler )
EVT_TIMER ( TIMER_THIS_PI, ropeless_pi::ProcessTimerEvent )
EVT_TIMER ( SIM_TIMER, ropeless_pi::ProcessSimTimerEvent )
END_EVENT_TABLE()

ropeless_pi::ropeless_pi( void *ppimgr ) :
        wxTimer( this ), opencpn_plugin_112( ppimgr )
{
    // Create the PlugIn icons
//    initialize_images();

    m_pplugin_icon = new wxBitmap(default_pi);

}

ropeless_pi::~ropeless_pi( void )
{
}

int ropeless_pi::Init( void )
{
    AddLocaleCatalog( _T("opencpn-ropeless_pi") );
    m_config_version = -1;

    m_oDC = NULL;

    //  Configure the NMEA processor
    mHDx_Watchdog = 2;
    mHDT_Watchdog = 2;
    mGPS_Watchdog = 2;
    mVar_Watchdog = 2;
    mPriPosition = 9;
    mPriDateTime = 9;
    mPriVar = 9;
    mPriHeadingM = 9;
    mPriHeadingT = 9;

    gHDT_Watchdog = 10;
    gGPS_Watchdog = 10;

    mVar = 0;
    m_hdt = 0;
    m_ownship_cog = 0;
    m_nfix = 0;
    m_bshow_fix_hat = false;

    m_FixHatColor = wxColour(0, 128, 128);
    m_Thread_run_flag = -1;

    g_iconTypeArray.Add(_T("Scaled Vector Icon"));
    g_iconTypeArray.Add(_T("Generic Ship Icon"));

    m_tenderGPS_y = 37.1;               // From bow
    m_tenderGPS_x  = 2.6;               // stbd from midships
    m_tenderLength = 41.1;
    m_tenderWidth = 10.7;

//     Length = 41.1
//     Beam = 10.7
//     GPS offset from bow = 37.1
//     GPS offset from midship = 2.6




    gHdt = NAN;
    gHdm = NAN;
    gVar = NAN;
    gSog = NAN;
    gCog = NAN;

    //    Get a pointer to the opencpn configuration object
    m_pconfig = GetOCPNConfigObject();

    //    And load the configuration items
    LoadConfig();

    m_pTrackRolloverWin = new RolloverWin( GetOCPNCanvasWindow() );
    m_pTrackRolloverWin->SetPosition( wxPoint( 5, 150 ) );
    m_pTrackRolloverWin->IsActive( false );

//     m_pTrackRolloverWin->SetString( _T("Brg:   0\nDist:   0") );
//     m_pTrackRolloverWin->SetBestSize();
//     m_pTrackRolloverWin->SetBitmap( 0 );
//     m_pTrackRolloverWin->SetPosition( wxPoint( 5, 50 ) );
//
//     m_pTrackRolloverWin->IsActive( true );


    SetOwner(this, TIMER_THIS_PI);
    Start( 1000, wxTIMER_CONTINUOUS );

#ifndef __WXMSW__
    wxEVT_PI_OCPN_DATASTREAM = wxNewEventType();
#endif

    m_event_handler = new PI_EventHandler(this);
    m_serialThread = NULL;

    //startSerial(m_serialPort);

    m_RolloverPopupTimer.SetOwner( m_event_handler, ROLLOVER_TIMER );
    m_rollover_popup_timer_msec = 20;
    m_pBrgRolloverWin = NULL;
    m_pRolloverBrg = NULL;

    m_select = new Select();
    m_tenderSelect = NULL;

    m_head_dog_timer.SetOwner( m_event_handler, HEAD_DOG_TIMER );
    m_head_active = false;

    setTrackedWPSelect(m_trackedWPGUID);


    ///  Testing
#if 0
    brg_line *brg = new brg_line(350, TRUE_BRG, 42.033, -70.183, 2.);
    m_brg_array.Add(brg);
    m_select->AddSelectablePoint( brg->GetLatA(), brg->GetLonA(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
    m_select->AddSelectablePoint( brg->GetLatB(), brg->GetLonB(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
    m_select->AddSelectableSegment( brg->GetLatA(), brg->GetLonA(), brg->GetLatB(), brg->GetLonB(), brg, SEL_SEG );

    brg = new brg_line(120, TRUE_BRG, 42.033, -70.183, 2.);
    m_brg_array.Add(brg);
    m_select->AddSelectablePoint( brg->GetLatA(), brg->GetLonA(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
    m_select->AddSelectablePoint( brg->GetLatB(), brg->GetLonB(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
    m_select->AddSelectableSegment( brg->GetLatA(), brg->GetLonA(), brg->GetLatB(), brg->GetLonB(), brg, SEL_SEG );

    brg = new brg_line(200, TRUE_BRG, 42.033, -70.183, 2.);
    m_brg_array.Add(brg);
    m_select->AddSelectablePoint( brg->GetLatA(), brg->GetLonA(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
    m_select->AddSelectablePoint( brg->GetLatB(), brg->GetLonB(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
    m_select->AddSelectableSegment( brg->GetLatA(), brg->GetLonA(), brg->GetLatB(), brg->GetLonB(), brg, SEL_SEG );
#endif

    m_colorIndexNext = 0;

    wxMenuItem *sim_item = new wxMenuItem(NULL, ID_PLAY_SIM, _("Start Ropeless Simulation") );
    m_start_sim_id = AddCanvasContextMenuItem(sim_item, this );
    SetCanvasContextMenuItemViz(m_start_sim_id, true);

    wxMenuItem *sim_stop_item = new wxMenuItem(NULL, ID_STOP_SIM, _("Stop Simulation") );
    m_stop_sim_id = AddCanvasContextMenuItem(sim_stop_item, this );
    SetCanvasContextMenuItemViz(m_stop_sim_id, false);

    return (WANTS_OVERLAY_CALLBACK |
            WANTS_OPENGL_OVERLAY_CALLBACK |
            WANTS_CURSOR_LATLON       |
            WANTS_TOOLBAR_CALLBACK    |
            INSTALLS_TOOLBAR_TOOL     |
            WANTS_CONFIG              |
            WANTS_PREFERENCES         |
            WANTS_PLUGIN_MESSAGING    |
            WANTS_NMEA_SENTENCES      |
            WANTS_NMEA_EVENTS         |
            WANTS_PREFERENCES         |
            WANTS_MOUSE_EVENTS        |
            INSTALLS_CONTEXTMENU_ITEMS
            );

}

bool ropeless_pi::DeInit( void )
{
    if( IsRunning() ) // Timer started?
        Stop(); // Stop timer

    stopSerial();

    delete m_event_handler;             // also diconnects serial events

//     int tsec = 2;
//     while(tsec--)
//         wxSleep(1);

    return true;
}


int ropeless_pi::GetAPIVersionMajor()
{
    return MY_API_VERSION_MAJOR;
}

int ropeless_pi::GetAPIVersionMinor()
{
    return MY_API_VERSION_MINOR;
}

int ropeless_pi::GetPlugInVersionMajor()
{
    return PLUGIN_VERSION_MAJOR;
}

int ropeless_pi::GetPlugInVersionMinor()
{
    return PLUGIN_VERSION_MINOR;
}

wxBitmap *ropeless_pi::GetPlugInBitmap()
{
    return m_pplugin_icon;
}

wxString ropeless_pi::GetCommonName()
{
    return _("Ropeless");
}

wxString ropeless_pi::GetShortDescription()
{
    return _("Ropeless PlugIn for OpenCPN");
}

wxString ropeless_pi::GetLongDescription()
{
    return _("Ropeless PlugIn for OpenCPN");

}

void ropeless_pi::OnContextMenuItemCallback(int id)
{

  if (id == m_start_sim_id){

    wxString file;
    int response = PlatformFileSelectorDialog(
        NULL, &file, _("Select an NMEA text file"), *GetpPrivateApplicationDataLocation(), _T(""), _T("*.*"));

    if (response == wxID_OK) {

      msgFileName = file;
      if(::wxFileExists(msgFileName)){

        SetCanvasContextMenuItemViz(m_start_sim_id, false);
        SetCanvasContextMenuItemViz(m_stop_sim_id, true);

        startSim();
      }
    }
  }
  else if (id == m_stop_sim_id){
    SetCanvasContextMenuItemViz(m_start_sim_id, true);
    SetCanvasContextMenuItemViz(m_stop_sim_id, false);

    stopSim();
  }


    //startSim();
}

void ropeless_pi::PopupMenuHandler( wxCommandEvent& event )
{
    bool handled = false;
    switch( event.GetId() ) {

        case ID_PLAY_SIM:
            startSim();

            handled = true;
            break;

        case ID_EPL_XMIT:
        {

            m_NMEA0183.TalkerID = _T("EC");

            SENTENCE snt;

            if( m_fix_lat < 0. )
                m_NMEA0183.Gll.Position.Latitude.Set( -m_fix_lat, _T("S") );
            else
                m_NMEA0183.Gll.Position.Latitude.Set( m_fix_lat, _T("N") );

            if( m_fix_lon < 0. )
                m_NMEA0183.Gll.Position.Longitude.Set( -m_fix_lon, _T("W") );
            else
                m_NMEA0183.Gll.Position.Longitude.Set( m_fix_lon, _T("E") );

            wxDateTime now = wxDateTime::Now();
            wxDateTime utc = now.ToUTC();
            wxString time = utc.Format( _T("%H%M%S") );
            m_NMEA0183.Gll.UTCTime = time;

            m_NMEA0183.Gll.Mode = _T("M");              // Use GLL 2.3 specification
                                                        // and send "M" for "Manual fix"
            m_NMEA0183.Gll.IsDataValid = NFalse;        // Spec requires "Invalid" for manual fix

            m_NMEA0183.Gll.Write( snt );

            wxLogMessage(snt.Sentence);
            PushNMEABuffer( snt.Sentence );

            handled = true;
            break;
        }

        default:
            break;
    }

    if(!handled)
        event.Skip();
}

bool ropeless_pi::MouseEventHook( wxMouseEvent &event )
{
    bool bret = false;
#if 0
    m_mouse_x = event.m_x;
    m_mouse_y = event.m_y;

    //  Retrigger the rollover timer
    if( m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive() )
        m_RolloverPopupTimer.Start( 10, wxTIMER_ONE_SHOT );               // faster response while the rollover is turned on
    else
        m_RolloverPopupTimer.Start( m_rollover_popup_timer_msec, wxTIMER_ONE_SHOT );

    wxPoint mp(event.m_x, event.m_y);
    GetCanvasLLPix( &g_ovp, mp, &m_cursor_lat, &m_cursor_lon);

    //  On button push, find any bearing line selecteable, and other useful data
    if( event.RightDown() || event.LeftDown()) {
        m_sel_brg = NULL;

        m_pFind = m_select->FindSelection( m_cursor_lat, m_cursor_lon, SELTYPE_POINT_GENERIC );

        if(m_pFind){
            for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
                brg_line *pb = m_brg_array.Item(i);

                if(m_pFind->m_pData1 == pb){
                    m_sel_brg = pb;
                    m_sel_part = m_pFind->GetUserData();
                    if(SEL_POINT_A == m_sel_part){
                        m_sel_pt_lat = pb->GetLatA();
                        m_sel_pt_lon = pb->GetLonA();
                    }
                    else {
                        m_sel_pt_lat = pb->GetLatB();
                        m_sel_pt_lon = pb->GetLonB();
                    }

                    break;
                }
            }
        }
        else{
            m_pFind = m_select->FindSelection( m_cursor_lat, m_cursor_lon, SELTYPE_SEG_GENERIC );

            if(m_pFind){
                for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
                    brg_line *pb = m_brg_array.Item(i);

                    if(m_pFind->m_pData1 == pb){
                        m_sel_brg = pb;
                        m_sel_part = m_pFind->GetUserData();

                        //  Get the mercator offsets from the cursor point to the brg "A" point
                        toSM_Plugin(m_cursor_lat, m_cursor_lon, pb->GetLatA(), pb->GetLonA(),
                                    &m_segdrag_ref_x, &m_segdrag_ref_y);
                        m_sel_pt_lat = m_cursor_lat;
                        m_sel_pt_lon = m_cursor_lon;

                    }
                }
            }
        }
    }

#endif

    if( event.RightDown() ) {
        //wxMenu* contextMenu = new wxMenu;
        //wxMenuItem *sim_item = new wxMenuItem(contextMenu, ID_PLAY_SIM, _("Start Simulation") );
        //contextMenu->Append(sim_item);
        //GetOCPNCanvasWindow()->Connect( ID_PLAY_SIM, wxEVT_COMMAND_MENU_SELECTED,
        //                                        wxCommandEventHandler( ropeless_pi::PopupMenuHandler ), NULL, this );

#if 0
        if( m_sel_brg || m_bshow_fix_hat){

            wxMenu* contextMenu = new wxMenu;

            wxMenuItem *brg_item = 0;
            wxMenuItem *fix_item = 0;

            if(!m_bshow_fix_hat && m_sel_brg){
                brg_item = new wxMenuItem(contextMenu, ID_EPL_DELETE, _("Delete Bearing") );
                contextMenu->Append(brg_item);
                GetOCPNCanvasWindow()->Connect( ID_EPL_DELETE, wxEVT_COMMAND_MENU_SELECTED,
                                                wxCommandEventHandler( ropeless_pi::PopupMenuHandler ), NULL, this );
            }

            if(m_bshow_fix_hat){
                wxMenuItem *fix_item = new wxMenuItem(contextMenu, ID_EPL_XMIT, _("Send sighted fix to device") );
                contextMenu->Append(fix_item);
                GetOCPNCanvasWindow()->Connect( ID_EPL_XMIT, wxEVT_COMMAND_MENU_SELECTED,
                                                wxCommandEventHandler( ropeless_pi::PopupMenuHandler ), NULL, this );
            }

            //   Invoke the drop-down menu
            GetOCPNCanvasWindow()->PopupMenu( contextMenu, m_mouse_x, m_mouse_y );

            if(brg_item){
                GetOCPNCanvasWindow()->Disconnect( ID_EPL_DELETE, wxEVT_COMMAND_MENU_SELECTED,
                                                   wxCommandEventHandler( ropeless_pi::PopupMenuHandler ), NULL, this );
            }

            if(fix_item){
                GetOCPNCanvasWindow()->Connect( ID_EPL_XMIT, wxEVT_COMMAND_MENU_SELECTED,
                                                wxCommandEventHandler( ropeless_pi::PopupMenuHandler ), NULL, this );
            }

            bret = true;                // I have eaten this event
        }
#endif
    }

    else if( event.LeftDown() ) {
    }

    else if(event.Dragging()){
#if 0
        if(m_sel_brg){
            if( (SEL_POINT_A == m_sel_part) || (SEL_POINT_B == m_sel_part)){
                double dx, dy;
                toSM_Plugin(m_cursor_lat, m_cursor_lon, m_sel_pt_lat, m_sel_pt_lon, &dx, &dy);
                double distance = sqrt( (dx * dx) + (dy * dy));

                double new_lat = m_sel_pt_lat;
                double new_lon = m_sel_pt_lon;

                double alpha = atan2(dx, dy);
                if(alpha < 0)
                    alpha += 2 * PI;

                double brg_perp = (m_sel_brg->GetBearingTrue() - 90) * PI / 180.;
                if(brg_perp < 0)
                    brg_perp += 2 * PI;

                double delta_alpha = alpha - brg_perp;
                if(delta_alpha < 0)
                    delta_alpha += 2 * PI;

                double move_dist = distance * sin(delta_alpha);

                double ndy = move_dist * cos(m_sel_brg->GetBearingTrue() * PI / 180.);
                double ndx = move_dist * sin(m_sel_brg->GetBearingTrue() * PI / 180.);

                fromSM_Plugin(ndx, ndy, m_sel_pt_lat, m_sel_pt_lon, &new_lat, &new_lon);

                //  Update the brg line parameters
                if(SEL_POINT_A == m_sel_part){
                    m_sel_brg->m_latA = new_lat;
                    m_sel_brg->m_lonA = new_lon;
                }
                else if(SEL_POINT_B == m_sel_part){
                    m_sel_brg->m_latB = new_lat;
                    m_sel_brg->m_lonB = new_lon;
                }

                m_sel_brg->CalcLength();

                // Update the selectable items
                if(m_pFind){
                    m_select->DeleteSelectableSegment( m_sel_brg, SELTYPE_SEG_GENERIC, SEL_SEG );
                    m_select->AddSelectableSegment( m_sel_brg->GetLatA(), m_sel_brg->GetLonA(),
                                                    m_sel_brg->GetLatB(), m_sel_brg->GetLonB(),
                                                    m_sel_brg, SEL_SEG );

                    m_pFind->m_slat = new_lat;
                    m_pFind->m_slon = new_lon;
                }

                GetOCPNCanvasWindow()->Refresh();
                bret = true;
            }
            else if(SEL_SEG == m_sel_part){

                 //  Get the Mercator offsets from original select point to this drag point
                 double dx, dy;
                 toSM_Plugin(m_cursor_lat, m_cursor_lon, m_sel_pt_lat, m_sel_pt_lon, &dx, &dy);

                 //  Add in the offsets to item point "A"
                 dx -= m_segdrag_ref_x;
                 dy -= m_segdrag_ref_y;

                 //   And calculate new position of item point "A"
                 double nlatA, nlonA;
                 fromSM_Plugin(dx, dy, m_sel_pt_lat, m_sel_pt_lon, &nlatA, &nlonA);

                 //  Set point "A"
                 m_sel_brg->m_latA = nlatA;
                 m_sel_brg->m_lonA = nlonA;

                 // Recalculate point "B"
                 m_sel_brg->CalcPointB();

                 // Update the selectable items
                 m_select->DeleteSelectablePoint( m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
                 m_select->DeleteSelectablePoint( m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
                 m_select->DeleteSelectableSegment( m_sel_brg, SELTYPE_SEG_GENERIC, SEL_SEG );

                 m_select->AddSelectablePoint( m_sel_brg->GetLatA(), m_sel_brg->GetLonA(), m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
                 m_select->AddSelectablePoint( m_sel_brg->GetLatB(), m_sel_brg->GetLonB(), m_sel_brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
                 m_select->AddSelectableSegment( m_sel_brg->GetLatA(), m_sel_brg->GetLonA(),
                                                 m_sel_brg->GetLatB(), m_sel_brg->GetLonB(),
                                                 m_sel_brg, SEL_SEG );


                 GetOCPNCanvasWindow()->Refresh();
                 bret = true;
            }
        }
#endif
    }

    if( event.LeftUp() ) {
#if 0
        if(m_sel_brg)
            bret = true;

        m_pFind = NULL;
        m_sel_brg = NULL;

        m_nfix = CalculateFix();
#endif
    }
    return bret;
}


void ropeless_pi::setTrackedWPSelect(wxString GUID)
{
    if(GUID.Length()){
        // if it is already set
        m_select->DeleteSelectablePoint( this, SELTYPE_POINT_GENERIC, SELTYPE_ROUTEPOINT );

        // Get the WP location

        bool bfound = false;
        wxArrayString guidArray = GetWaypointGUIDArray();
        for(unsigned int i=0 ; i < guidArray.GetCount() ; i++){
            if(GUID.IsSameAs( guidArray[i] )){
                if(GetSingleWaypoint( GUID, &m_TrackedWP )){
                    bfound = true;
                    break;
                }
            }
        }

        if(bfound){
            m_select->AddSelectablePoint( m_TrackedWP.m_lat, m_TrackedWP.m_lon, this, SELTYPE_ROUTEPOINT, 0 );
        }
    }
}

int n_tick;
double countRun;
double countTarget;
double accelFactor;
wxString pendingMsg;
unsigned int inext;
unsigned int msgCount;
wxTextFile msgFile;
double tstamp_current;

void ropeless_pi::startSim()
{
    // Open the data file
  msgFile.Open(msgFileName);
  msgCount = msgFile.GetLineCount();
  inext = 0;
  n_tick = 0;

  countRun = 0;
  countTarget = 5;
  accelFactor = 20;

  m_simulatorTimer.SetOwner( this, SIM_TIMER );
  m_simulatorTimer.Start(100, wxTIMER_CONTINUOUS);

}

void ropeless_pi::stopSim()
{
  m_simulatorTimer.Stop();

}


void ropeless_pi::ProcessSimTimerEvent( wxTimerEvent& event )
{
  n_tick++;
  countRun += .100 * accelFactor;     // 100 msec basic timer

  if(countRun < countTarget){
    if( (n_tick %10) == 0){            // once per second
      printf("next msg in: %g\n", countTarget-countRun);
    }
  }
  else{
    //  Send the pending msg
    SetNMEASentence( pendingMsg );
    RequestRefresh(GetOCPNCanvasWindow());

    // Fetch the next message
    if(inext < msgCount){
      pendingMsg = msgFile.GetLine(inext);
      pendingMsg.Append("\r\n");
      inext++;

      double tstamp_last = tstamp_current;

      // parse the pending msg to get the timestamp
      m_NMEA0183 << pendingMsg;

      if( m_NMEA0183.PreParse() ) {
        if( m_NMEA0183.Parse() ){
          tstamp_current = m_NMEA0183.Rfa.TimeStamp;

          if(inext > 1)
            countTarget = (tstamp_current - tstamp_last) * 3600 * 24;
          countRun = 0;
        }
      }
    }
    else{
      SetCanvasContextMenuItemViz(m_start_sim_id, true);
      SetCanvasContextMenuItemViz(m_stop_sim_id, false);

      stopSim();
    }
  }
}



void ropeless_pi::ProcessTimerEvent( wxTimerEvent& event )
{
#if 0
    if(event.GetId() == ROLLOVER_TIMER)
        OnRolloverPopupTimerEvent( event );
    else if(event.GetId() == HEAD_DOG_TIMER)    // no hdt source available
        m_head_active = false;

    else{
        if(gGPS_Watchdog++ > 5)
            pos_valid = false;

        //  Every tick, get a fresh copy of the tracked waypoint, in case it got moved.
        GetSingleWaypoint( m_trackedWPGUID, &m_TrackedWP );

        //  Update the continuos tracking popup
        double brg, dist;
        DistanceBearingMercator(  m_TrackedWP.m_lat, m_TrackedWP.m_lon, gLat, gLon, &brg, &dist );

        if(dist < 1.0){
            wxString s;
            s << wxString::Format( wxString("Tracked Bearing:  %03d°  \n", wxConvUTF8 ), (int)brg  );

            if(dist > 0.5)
                s << wxString::Format( wxString("Tracked Distance: %2.1f NMi  ", wxConvUTF8 ), dist  );
            else
                s << wxString::Format( wxString("Tracked Distance: %4d Ft.  ", wxConvUTF8 ), (int)(dist * 6076.0)  );

            m_pTrackRolloverWin->SetString( s );
            m_pTrackRolloverWin->SetBestSize();
            m_pTrackRolloverWin->SetBitmap( 0 );
            m_pTrackRolloverWin->SetPosition( wxPoint( 5, 150 ) );

            m_pTrackRolloverWin->IsActive( true );
        }
        else
            m_pTrackRolloverWin->IsActive( false );


        m_select->DeleteSelectablePoint( this, SELTYPE_POINT_GENERIC, SELTYPE_ROUTEPOINT );
        m_select->AddSelectablePoint( m_TrackedWP.m_lat, m_TrackedWP.m_lon, this, SELTYPE_ROUTEPOINT, 0 );
    }
#endif

    // Age the transponder history records
    for (unsigned int i = 0 ; i < transponderStatus.size() ; i++){
      transponder_state *state = transponderStatus[i];

      std::deque<transponder_state_history *>::iterator it = state->historyQ.begin();
      while (it != state->historyQ.end()){
        transponder_state_history *tsh = *it++;
        if (tsh->tsh_timer_age > 0)
          tsh->tsh_timer_age--;
      }
    }

    RequestRefresh(GetOCPNCanvasWindow());

}


void ropeless_pi::OnRolloverPopupTimerEvent( wxTimerEvent& event )
{
    bool b_need_refresh = false;

    bool showRollover = false;



    if( (NULL == m_pRolloverBrg) ) {

        m_pFind = m_select->FindSelection( m_cursor_lat, m_cursor_lon, SELTYPE_ROUTEPOINT );

        if(m_pFind){
//             for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
//                 brg_line *pb = m_brg_array.Item(i);
//
//                 if(m_pFind->m_pData1 == pb){
//                     m_pRolloverBrg = pb;
//                     break;
//                 }
//             }
//         }
//
//         if( m_pRolloverBrg )
//         {
                showRollover = true;

                if( NULL == m_pBrgRolloverWin ) {
                    m_pBrgRolloverWin = new RolloverWin( GetOCPNCanvasWindow() );
                    m_pBrgRolloverWin->IsActive( false );
                }

                if( !m_pBrgRolloverWin->IsActive() ) {
                    wxString s;

//                    s = _T("Test Rollover");

                    double brg, dist;
                    DistanceBearingMercator(  m_TrackedWP.m_lat, m_TrackedWP.m_lon, gLat, gLon, &brg, &dist );

                    s << wxString::Format( wxString("Tracked Bearing:  %03d°  \n", wxConvUTF8 ), (int)brg  );

                    if(dist > 0.5)
                        s << wxString::Format( wxString("Tracked Distance: %2.1f NMi  ", wxConvUTF8 ), dist  );
                    else
                        s << wxString::Format( wxString("Tracked Distance: %4d Ft.  ", wxConvUTF8 ), (int)(dist * 6076.0)  );


                    m_pBrgRolloverWin->SetString( s );

                    m_pBrgRolloverWin->SetBestPosition( m_mouse_x, m_mouse_y, 16, 16, 0, wxSize(100, 100));
                    m_pBrgRolloverWin->SetBitmap( 0 );
                    m_pBrgRolloverWin->IsActive( true );
                    b_need_refresh = true;
                    showRollover = true;
                }
            }

    } else {
        //    Is the cursor still in select radius?
        if( !m_select->IsSelectableSegmentSelected( m_cursor_lat, m_cursor_lon, m_pFind ) )
            showRollover = false;
        else
            showRollover = true;
    }



    if( m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive() && !showRollover ) {
        m_pBrgRolloverWin->IsActive( false );
        m_pRolloverBrg = NULL;
        m_pBrgRolloverWin->Destroy();
        m_pBrgRolloverWin = NULL;
        b_need_refresh = true;
    } else if( m_pBrgRolloverWin && showRollover ) {
        m_pBrgRolloverWin->IsActive( true );
        b_need_refresh = true;
    }

    if( b_need_refresh )
        GetOCPNCanvasWindow()->Refresh(true);

}

void ropeless_pi::setIcon( char ** xpm_bits)
{
    m_tender_bmp = wxBitmap(xpm_bits);
    wxMask *mask = new wxMask(m_tender_bmp, wxColour(0,0,0));
    m_tender_bmp.SetMask(mask);

    m_iconTexture = 0;
}

void ropeless_pi::ComputeShipScaleFactor(float icon_hdt,
                                         int ownShipWidth, int ownShipLength,
                                         wxPoint &lShipMidPoint,
                                         wxPoint &GPSOffsetPixels, wxPoint lGPSPoint,
                                         float &scale_factor_x, float &scale_factor_y)
{
    float screenResolution = (float) ::wxGetDisplaySize().x / ::wxGetDisplaySizeMM().x ;

    //  Calculate the true ship length in exact pixels
    double ship_bow_lat, ship_bow_lon;
    ll_gc_ll( gLat, gLon, icon_hdt, m_tenderLength / 1852., &ship_bow_lat, &ship_bow_lon );
    wxPoint lShipBowPoint;
    wxPoint b_point;
    GetCanvasPixLL(g_vp, &b_point, ship_bow_lat, ship_bow_lon);
    wxPoint a_point;
    GetCanvasPixLL(g_vp, &a_point, gLat, gLon);

    float shipLength_px = sqrtf( powf( (float) (b_point.x - a_point.x), 2) +
    powf( (float) (b_point.y - a_point.y), 2) );

    //  And in mm
    float shipLength_mm = shipLength_px / screenResolution;

    //  Set minimum ownship drawing size
    float ownship_min_mm = 6;
    ownship_min_mm = wxMax(ownship_min_mm, 1.0);

    //  Calculate Nautical Miles distance from midships to gps antenna
    float hdt_ant = icon_hdt + 180.;
    float dy = ( m_tenderLength / 2 - m_tenderGPS_y ) / 1852.;
    float dx = m_tenderGPS_x / 1852.;
    if( m_tenderGPS_y > m_tenderLength / 2 )      //reverse?
    {
        hdt_ant = icon_hdt;
        dy = -dy;
    }

    //  If the drawn ship size is going to be clamped, adjust the gps antenna offsets
    if( shipLength_mm < ownship_min_mm ) {
        dy /= shipLength_mm / ownship_min_mm;
        dx /= shipLength_mm / ownship_min_mm;
    }

    double ship_mid_lat, ship_mid_lon, ship_mid_lat1, ship_mid_lon1;

    ll_gc_ll( gLat, gLon, hdt_ant, dy, &ship_mid_lat, &ship_mid_lon );
    ll_gc_ll( ship_mid_lat, ship_mid_lon, icon_hdt - 90., dx, &ship_mid_lat1, &ship_mid_lon1 );

    GetCanvasPixLL(g_vp, &lShipMidPoint, ship_mid_lat1, ship_mid_lon1);
    GPSOffsetPixels.x = lShipMidPoint.x - lGPSPoint.x;
    GPSOffsetPixels.y = lShipMidPoint.y - lGPSPoint.y;

    float scale_factor = shipLength_px / ownShipLength;

    //  Calculate a scale factor that would produce a reasonably sized icon
    float scale_factor_min = ownship_min_mm / ( ownShipLength / screenResolution );

    //  And choose the correct one
    scale_factor = wxMax(scale_factor, scale_factor_min);

    scale_factor_y = scale_factor;
    scale_factor_x = scale_factor_y * ( (float) ownShipLength / ownShipWidth )
    / ( (float) m_tenderLength / m_tenderWidth );
}


void ropeless_pi::RenderIconDC(wxDC &dc )
{
    wxPoint ab;
    GetCanvasPixLL(g_vp, &ab, gLat, gLon);

    //  Draw the icon rotated to the COG
    //  or to the Hdt if available
    float icon_hdt = gCog;

    if( wxIsNaN( gHdt ) ){
        if(!wxIsNaN(gHdm) && !wxIsNaN(gVar)){
            icon_hdt = gHdm + gVar;
        }
    }
    else
        icon_hdt = gHdt;

    //  COG may be undefined in NMEA data stream
    if( wxIsNaN(icon_hdt) ) icon_hdt = 0.0;

    //    Calculate the ownship drawing angle icon_rad using an assumed 10 minute predictor
    double osd_head_lat, osd_head_lon;
    wxPoint osd_head_point;

    ll_gc_ll( gLat, gLon, icon_hdt, gSog * 10. / 60., &osd_head_lat, &osd_head_lon );

    wxPoint lShipMidPoint;
    GetCanvasPixLL(g_vp, &lShipMidPoint, gLat, gLon);
    GetCanvasPixLL(g_vp, &osd_head_point, osd_head_lat, osd_head_lon);


    float icon_rad = atan2f( (float) ( osd_head_point.y - lShipMidPoint.y ),
                             (float) ( osd_head_point.x - lShipMidPoint.x ) );
    icon_rad += (float)PI;
    double rotate = g_vp->rotation;

    if (gSog < 0.2) icon_rad = ((icon_hdt + 90.) * PI / 180) + rotate;


    float scale_factor_x, scale_factor_y;
    int ownShipWidth = 22; // Default values from s_ownship_icon
    int ownShipLength= 84;
    wxPoint lGPSPoint,  lPredPoint, lHeadPoint, GPSOffsetPixels(0,0);

    GetCanvasPixLL(g_vp, &lGPSPoint, gLat, gLon);

    ComputeShipScaleFactor(icon_hdt, ownShipWidth, ownShipLength,
                           lShipMidPoint, GPSOffsetPixels, lGPSPoint, scale_factor_x, scale_factor_y);

    if(m_tenderIconType.IsSameAs(_T("Generic Ship Icon")))
        dc.DrawBitmap(m_tender_bmp, ab.x, ab.y, true);
    else{

        wxPoint ownship_icon[10];

        for( int i = 0; i < 10; i++ ) {
            int j = i * 2;
            float pxa = (float) ( s_ownship_icon[j] );
            float pya = (float) ( s_ownship_icon[j + 1] );
            pya *= scale_factor_y;
            pxa *= scale_factor_x;

            float px = ( pxa * sinf( icon_rad ) ) + ( pya * cosf( icon_rad ) );
            float py = ( pya * sinf( icon_rad ) ) - ( pxa * cosf( icon_rad ) );

            ownship_icon[i].x = (int) ( px ) + lShipMidPoint.x;
            ownship_icon[i].y = (int) ( py ) + lShipMidPoint.y;
        }

        wxColour pcol;
        GetGlobalColor( _T ( "UBLCK" ), &pcol);
        wxPen ppPen1( pcol, 1, wxPENSTYLE_SOLID );
        dc.SetPen( ppPen1 );

        wxColour tenderColor;
        GetGlobalColor( _T ( "UGREN" ), &tenderColor);
        if(pos_valid){
             dc.SetBrush( wxBrush( tenderColor ) );
        }
        else
            dc.SetBrush( *wxTRANSPARENT_BRUSH );



        dc.DrawPolygon( 6, &ownship_icon[0], 0, 0 );

        dc.DrawLine( ownship_icon[6].x, ownship_icon[6].y, ownship_icon[7].x, ownship_icon[7].y );
        dc.DrawLine( ownship_icon[8].x, ownship_icon[8].y, ownship_icon[9].x, ownship_icon[9].y );

        //      Reference point, where the GPS antenna is
        int circle_rad = 3;

        dc.SetPen( wxPen( pcol, 1 ) );
        dc.SetBrush( wxBrush( pcol ) );
        dc.DrawCircle( lGPSPoint.x, lGPSPoint.y, circle_rad );

//         dc.StrokePolygon( 6, &ownship_icon[0], 0, 0 );
//
//         //     draw reference point (midships) cross
//         dc.StrokeLine( ownship_icon[6].x, ownship_icon[6].y, ownship_icon[7].x,
//                        ownship_icon[7].y );
//         dc.StrokeLine( ownship_icon[8].x, ownship_icon[8].y, ownship_icon[9].x,
//                       ownship_icon[9].y );
    }


}

void ropeless_pi::RenderIconGL( )
{
#if 0
       wxPoint ab;
    GetCanvasPixLL(g_vp, &ab, gLat, gLon);

    //  Draw the icon rotated to the COG
    //  or to the Hdt if available
    float icon_hdt = gCog;

    if( wxIsNaN( gHdt ) ){
        if(!wxIsNaN(gHdm) && !wxIsNaN(gVar)){
            icon_hdt = gHdm + gVar;
        }
    }
    else
        icon_hdt = gHdt;

    //  COG may be undefined in NMEA data stream
    if( wxIsNaN(icon_hdt) ) icon_hdt = 0.0;

    //    Calculate the ownship drawing angle icon_rad using an assumed 10 minute predictor
    double osd_head_lat, osd_head_lon;
    wxPoint osd_head_point;

    ll_gc_ll( gLat, gLon, icon_hdt, gSog * 10. / 60., &osd_head_lat, &osd_head_lon );

    wxPoint lShipMidPoint;
    GetCanvasPixLL(g_vp, &lShipMidPoint, gLat, gLon);
    GetCanvasPixLL(g_vp, &osd_head_point, osd_head_lat, osd_head_lon);


    float icon_rad = atan2f( (float) ( osd_head_point.y - lShipMidPoint.y ),
                             (float) ( osd_head_point.x - lShipMidPoint.x ) );
    icon_rad += (float)PI;
    double rotate = g_vp->rotation;

    if (gSog < 0.2) icon_rad = ((icon_hdt + 90.) * PI / 180) + rotate;


    float scale_factor_x, scale_factor_y;
    int ownShipWidth = 22; // Default values from s_ownship_icon
    int ownShipLength= 84;
    wxPoint lGPSPoint,  lPredPoint, lHeadPoint, GPSOffsetPixels(0,0);

    GetCanvasPixLL(g_vp, &lGPSPoint, gLat, gLon);

    ComputeShipScaleFactor(icon_hdt, ownShipWidth, ownShipLength,
                           lShipMidPoint, GPSOffsetPixels, lGPSPoint, scale_factor_x, scale_factor_y);

    if(m_tenderIconType.IsSameAs(_T("Generic Ship Icon"))){
        if(m_iconTexture == 0){
            m_iconTexture = GetIconTexture( &m_tender_bmp, m_texwidth, m_texheight );
        }

        wxPoint ab;
        GetCanvasPixLL(g_vp, &ab, gLat, gLon);

        renderTexture( m_iconTexture, ab, m_texwidth, m_texheight );

    }
    else{

        glEnable(GL_BLEND);

        glEnableClientState(GL_VERTEX_ARRAY);

        int x = lShipMidPoint.x, y = lShipMidPoint.y;
        glPushMatrix();
        glTranslatef(x, y, 0);

        float deg = 180/PI * ( icon_rad - PI/2 );
        glRotatef(deg, 0, 0, 1);

        glScalef(scale_factor_x, scale_factor_y, 1);

        { // Scaled Vector

            static const GLint s_ownship_icon[] = { 5, -42, 11, -28, 11, 42, -11, 42,
            -11, -28, -5, -42, -11, 0, 11, 0,
            0, 42, 0, -42       };

            glColor4ub(0, 255, 0, 255);

            glVertexPointer(2, GL_INT, 2*sizeof(GLint), s_ownship_icon);

            if(pos_valid)
                glDrawArrays(GL_POLYGON, 0, 6);

            glColor4ub(0, 0, 0, 255);
            glLineWidth(2);

            glDrawArrays(GL_LINE_LOOP, 0, 6);
            glDrawArrays(GL_LINES, 6, 4);
        }
        glPopMatrix();


        //      Reference point, where the GPS antenna is
        int circle_rad = 3;

        float cx = lGPSPoint.x, cy = lGPSPoint.y;
        // store circle coordinates at compile time
        const int v = 12;
        float circle[4*v];
        for( int i=0; i<2*v; i+=2) {
            float a = i * (float)PI / v;
            float s = sinf( a ), c = cosf( a );
            circle[i+0] = cx + (circle_rad+1) * s;
            circle[i+1] = cy + (circle_rad+1) * c;
            circle[i+2*v] = cx + circle_rad * s;
            circle[i+2*v+1] = cy + circle_rad * c;
        }

        glVertexPointer(2, GL_FLOAT, 2*sizeof(float), circle);

        glColor4ub(0, 0, 0, 255);
        glDrawArrays(GL_TRIANGLE_FAN, 0, v);
        glColor4ub(255, 255, 255, 255);
        glDrawArrays(GL_TRIANGLE_FAN, v, v);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisable( GL_LINE_SMOOTH );
        glDisable( GL_POLYGON_SMOOTH );
        glDisable(GL_BLEND);


    }

#endif
}

void ropeless_pi::RenderTransponder( transponder_state *state)
{
    int circle_size = 10;

    wxPoint ab;
    wxString colorName = colorTableNames[state->color_index];
    wxColour rcolour = wxTheColourDatabase->Find(colorName);
    if (!rcolour.IsOk())
      rcolour = wxColour(255,000,255);

    // Render the primary instant transponder
    GetCanvasPixLL(g_vp, &ab, state->predicted_lat, state->predicted_lon);
    wxPen dpen( rcolour );
    wxBrush dbrush( rcolour );
    m_oDC->SetPen( dpen );
    m_oDC->SetBrush( dbrush );
    m_oDC->DrawCircle( ab.x, ab.y, circle_size);


#if 1
    // Render the history buffer, if present
    std::deque<transponder_state_history *>::iterator it = state->historyQ.begin();
    while (it != state->historyQ.end()){
        transponder_state_history *tsh = *it++;

        wxPoint abh;
        GetCanvasPixLL(g_vp, &abh, tsh->predicted_lat, tsh->predicted_lon);

        wxColour hcolour(rcolour.Red(), rcolour.Green(), rcolour.Blue(),
                         (tsh->tsh_timer_age / HISTORY_FADE_SECS) * 254);
        wxPen dpen( hcolour );
        wxBrush dbrush( hcolour );
        m_oDC->SetPen( dpen );
        m_oDC->SetBrush( dbrush );
        m_oDC->DrawCircle( abh.x, abh.y, circle_size/2);
    }
#endif
    // Draw an "X" over the prinary target
    wxPoint x1(ab.x - circle_size * .707, ab.y - circle_size * .707);
    wxPoint x2(ab.x + circle_size * .707, ab.y + circle_size * .707);
    wxPoint x3(ab.x - circle_size * .707, ab.y + circle_size * .707);
    wxPoint x4(ab.x + circle_size * .707, ab.y - circle_size * .707);
    wxPen xpen( *wxBLACK, 3 );
    m_oDC->SetPen( xpen );
    m_oDC->DrawLine(x1.x, x1.y, x2.x, x2.y, true);
    m_oDC->DrawLine(x3.x, x3.y, x4.x, x4.y, true);

}




void ropeless_pi::RenderTrawls()
{
    //  Walk the vector of transponcer status
  for (unsigned int i = 0 ; i < transponderStatus.size() ; i++){
    transponder_state *state = transponderStatus[i];

    RenderTransponder(state);

  }
}


bool ropeless_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp)
{
#if 0
    g_vp = vp;
    Clone_VP(&g_ovp, vp);                // deep copy

    if (!m_oDC) m_oDC = new ODDC(dc);

    m_oDC->SetVP(vp);



    double selec_radius = (10 / vp->view_scale_ppm) / (1852. * 60.);
    m_select->SetSelectLLRadius(selec_radius);


// #if wxUSE_GRAPHICS_CONTEXT
//     wxMemoryDC *pmdc;
//     pmdc = wxDynamicCast(&dc, wxMemoryDC);
//     wxGraphicsContext *pgc = wxGraphicsContext::Create( *pmdc );
//     g_gdc = pgc;
//     g_pdc = &dc;
// #else
//     g_pdc = &dc;
// #endif

    //Render
    //RenderTrawls();
    //RenderIconDC( dc );


//#if wxUSE_GRAPHICS_CONTEXT
//    delete g_gdc;
//#endif

#if 0
    if( m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive() ) {
        dc.DrawBitmap( *(m_pBrgRolloverWin->GetBitmap()),
                       m_pBrgRolloverWin->GetPosition().x,
                       m_pBrgRolloverWin->GetPosition().y, false );
    }

    if( m_pTrackRolloverWin && m_pTrackRolloverWin->IsActive() ) {
        dc.DrawBitmap( *(m_pTrackRolloverWin->GetBitmap()),
                       m_pTrackRolloverWin->GetPosition().x,
                       m_pTrackRolloverWin->GetPosition().y, false );
    }


#endif
#endif
    return true;
}

bool ropeless_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp)
{
    if (!m_oDC) m_oDC = new ODDC();

    m_oDC->SetVP(vp);


//     g_gdc = NULL;
//     g_pdc = NULL;

    g_vp = vp;
    Clone_VP(&g_ovp, vp);                // deep copy

    double selec_radius = (10 / vp->view_scale_ppm) / (1852. * 60.);
    m_select->SetSelectLLRadius(selec_radius);

    //Render
    RenderTrawls();
#if 0
    RenderIconGL(  );

    if( m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive() ) {
        wxImage image = m_pBrgRolloverWin->GetBitmap()->ConvertToImage();
        unsigned char *imgdata = image.GetData();
        if(imgdata){
            glRasterPos2i(m_pBrgRolloverWin->GetPosition().x, m_pBrgRolloverWin->GetPosition().y);

            glPixelZoom(1.0, -1.0);
            glDrawPixels(image.GetWidth(), image.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE,imgdata);
            glPixelZoom(1.0, 1.0);
        }

    }

    if( m_pTrackRolloverWin && m_pTrackRolloverWin->IsActive() ) {
        wxImage image = m_pTrackRolloverWin->GetBitmap()->ConvertToImage();
        unsigned char *imgdata = image.GetData();
        if(imgdata){
            glRasterPos2i(m_pTrackRolloverWin->GetPosition().x, m_pTrackRolloverWin->GetPosition().y);

            glPixelZoom(1.0, -1.0);
            glDrawPixels(image.GetWidth(), image.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE,imgdata);
            glPixelZoom(1.0, 1.0);
        }

    }
#endif
    return true;
}


void ropeless_pi::ProcessRFACapture( void )
{
  if( m_NMEA0183.LastSentenceIDReceived != _T("RFA") )
    return;

  int transponderIdent = m_NMEA0183.Rfa.TransponderCode;

  // Check if status for this transponder is in the status vector
  transponder_state *this_transponder_state = NULL;
  for (unsigned int i = 0 ; i < transponderStatus.size() ; i++){
    if (transponderStatus[i]->ident == transponderIdent){
      this_transponder_state = transponderStatus[i];
      break;
    }
  }

  // If not present, create a new record, and add to vector
  if (this_transponder_state == NULL){
    this_transponder_state = new transponder_state;

    // Select a new color for this new transponder
    this_transponder_state->color_index = m_colorIndexNext;
    m_colorIndexNext++;
    if(m_colorIndexNext == COLOR_TABLE_COUNT)
      m_colorIndexNext = 0;

    transponderStatus.push_back( this_transponder_state );
  }
  else {                         // Maintain history buffer
    transponder_state_history *this_transponder_state_history = new transponder_state_history;
    this_transponder_state_history->ident = this_transponder_state->ident;
    this_transponder_state_history->timeStamp = this_transponder_state->timeStamp;
    this_transponder_state_history->predicted_lat = this_transponder_state->predicted_lat;
    this_transponder_state_history->predicted_lon = this_transponder_state->predicted_lon;
    this_transponder_state_history->color_index = this_transponder_state->color_index;
    this_transponder_state_history->tsh_timer_age = HISTORY_FADE_SECS;

    if (this_transponder_state->historyQ.size() > 10){
        this_transponder_state->historyQ.pop_back();
    }
    this_transponder_state->historyQ.push_front(this_transponder_state_history);
  }

  //  Update the instant record
  this_transponder_state->ident = transponderIdent;
  this_transponder_state->timeStamp = m_NMEA0183.Rfa.TimeStamp;

  this_transponder_state->predicted_lat = m_NMEA0183.Rfa.TransponderPosition.Latitude.Latitude;
      if (m_NMEA0183.Rfa.TransponderPosition.Latitude.Northing == South)
        this_transponder_state->predicted_lat = -this_transponder_state->predicted_lat;

  this_transponder_state->predicted_lon = m_NMEA0183.Rfa.TransponderPosition.Longitude.Longitude;
  if (m_NMEA0183.Rfa.TransponderPosition.Longitude.Easting == West)
    this_transponder_state->predicted_lon = -this_transponder_state->predicted_lon;



}


void ropeless_pi::SetNMEASentence( wxString &sentence )
{
    if (sentence.IsEmpty())
      return;

    m_NMEA0183 << sentence;

    if( m_NMEA0183.PreParse() ) {
        if( m_NMEA0183.LastSentenceIDReceived == _T("RFA") ) {
            if( m_NMEA0183.Parse() )
                ProcessRFACapture();
        }

#if 0
        else if( m_NMEA0183.LastSentenceIDReceived == _T("HDG") ) {
            if( m_NMEA0183.Parse() ) {

                    if( !wxIsNaN( m_NMEA0183.Hdg.MagneticVariationDegrees ) ){
                        if( m_NMEA0183.Hdg.MagneticVariationDirection == East )
                            mVar =  m_NMEA0183.Hdg.MagneticVariationDegrees;
                        else if( m_NMEA0183.Hdg.MagneticVariationDirection == West )
                            mVar = -m_NMEA0183.Hdg.MagneticVariationDegrees;
                    }

                    if( !wxIsNaN(m_NMEA0183.Hdg.MagneticSensorHeadingDegrees) ) {
                        if( !wxIsNaN( mVar ) ){
                            m_hdt = m_NMEA0183.Hdg.MagneticSensorHeadingDegrees + mVar;
                            m_head_active = true;
                            m_head_dog_timer.Start(5000, wxTIMER_ONE_SHOT);
                        }
                    }

            }
        }

        else if( m_NMEA0183.LastSentenceIDReceived == _T("HDM") ) {
            if( m_NMEA0183.Parse() ) {

                if( !wxIsNaN(m_NMEA0183.Hdm.DegreesMagnetic) ) {
                    if( !wxIsNaN( mVar ) ){
                        m_hdt = m_NMEA0183.Hdm.DegreesMagnetic + mVar;
                        m_head_active = true;
                        m_head_dog_timer.Start(5000, wxTIMER_ONE_SHOT);
                    }
                }

            }
        }

        else if( m_NMEA0183.LastSentenceIDReceived == _T("HDT") ) {
            if( m_NMEA0183.Parse() ) {
                if( mPriHeadingT >= 1 ) {
                    mPriHeadingT = 1;
                    if( m_NMEA0183.Hdt.DegreesTrue < 999. ) {
                        m_hdt = m_NMEA0183.Hdt.DegreesTrue;
                        m_head_active = true;
                        m_head_dog_timer.Start(5000, wxTIMER_ONE_SHOT);
                    }
                }
            }
        }

#endif

    }
}

void ropeless_pi::SetPositionFix( PlugIn_Position_Fix &pfix )
{
    if(1){
        m_ownship_lat = pfix.Lat;
        m_ownship_lon = pfix.Lon;
        if(!wxIsNaN( pfix.Cog ))
            m_ownship_cog = pfix.Cog;
        if(!wxIsNaN( pfix.Var ))
            mVar = pfix.Var;
    }
}

void ropeless_pi::SetCursorLatLon( double lat, double lon )
{
}

void ropeless_pi::SetPluginMessage(wxString &message_id, wxString &message_body)
{
}

int ropeless_pi::GetToolbarToolCount( void )
{
    return 1;
}


void ropeless_pi::SetColorScheme( PI_ColorScheme cs )
{
}


void ropeless_pi::OnToolbarToolCallback( int id )
{
}


bool ropeless_pi::LoadConfig( void )
{

    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;

    if( pConf ) {

        pConf->SetPath ( _T( "/Settings/Ropeless_pi" ) );
        pConf->Read ( _T( "SerialPort" ),  &m_serialPort );
        pConf->Read ( _T( "TrackedPoint" ),  &m_trackedWPGUID );

        m_trackedWP = getWaypointName(m_trackedWPGUID);

        pConf->Read ( _T( "TenderLength" ),  &m_tenderLength );
        pConf->Read ( _T( "TenderWidth" ),  &m_tenderWidth );
        pConf->Read ( _T( "TenderGPSOffsetX" ),  &m_tenderGPS_x );
        pConf->Read ( _T( "TenderGPSOffsetY" ),  &m_tenderGPS_y );

        pConf->Read ( _T( "TenderIconType" ),  &m_tenderIconType );


        // Qualify the input
        m_tenderLength = wxMax(1, m_tenderLength);
        m_tenderWidth = wxMax(1, m_tenderWidth);

        bool bfound = false;
        for(unsigned int i=0 ; i < g_iconTypeArray.Count() ; i++){
            if(m_tenderIconType.IsSameAs(g_iconTypeArray.Item(i))){
                bfound = true;
                break;
            }
        }
        if(!bfound)
            m_tenderIconType = g_iconTypeArray.Item(0);


        //setIcon( shippity );

        return true;
    } else
        return false;
}

bool ropeless_pi::SaveConfig( void )
{
    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;

    if( pConf ) {

        pConf->SetPath ( _T( "/Settings/Ropeless_pi" ) );
        pConf->Write ( _T( "SerialPort" ),  m_serialPort );
        pConf->Write ( _T( "TrackedPoint" ),  m_trackedWPGUID );

        pConf->Write ( _T( "TenderLength" ),  m_tenderLength );
        pConf->Write ( _T( "TenderWidth" ),  m_tenderWidth );
        pConf->Write ( _T( "TenderGPSOffsetX" ),  m_tenderGPS_x );
        pConf->Write ( _T( "TenderGPSOffsetY" ),  m_tenderGPS_y );
        pConf->Write ( _T( "TenderIconType" ),  m_tenderIconType );

        return true;
    } else
        return false;
}

void ropeless_pi::ApplyConfig( void )
{

}

#if 0
void ropeless_pi::ProcessBrgCapture(double brg_rel, double brg_subtended, double brg_TM, int brg_TM_flag,
                           wxString Ident, wxString target)
{
    double brg_true = 0;
    BearingTypeEnum type = TRUE_BRG;

    double lat = m_ownship_lat;          // initial declaration causes bearling line to pass thru ownship
    double lon = m_ownship_lon;

    //  What initial length?
    //  say 20% of canvas horizontal size, and max of 10 NMi

    double l1 = ((g_ovp.pix_width / g_ovp.view_scale_ppm) /1852.) * 0.2;
    double length = wxMin(l1, 10.0);


    // Process the normal case where the bearing is ship-head relative

    //  If we don't have a true heading available, use ownship cog
    if(m_head_active)
        brg_true = brg_rel + m_hdt;
    else
        brg_true = brg_rel + m_ownship_cog;

    type = TRUE_BRG;

    //  Do not add duplicates
    bool b_add = true;
    for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
        brg_line *pb = m_brg_array.Item(i);
        if(pb->GetIdent() == Ident){
            b_add = false;
            break;
        }
    }

    if(b_add){
        brg_line *brg = new brg_line(brg_true, type, lat, lon, length);
        brg->SetIdent(Ident);
        brg->SetTargetName(target);

        m_brg_array.Add(brg);
        m_select->AddSelectablePoint( brg->GetLatA(), brg->GetLonA(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_A );
        m_select->AddSelectablePoint( brg->GetLatB(), brg->GetLonB(), brg, SELTYPE_POINT_GENERIC, SEL_POINT_B );
        m_select->AddSelectableSegment( brg->GetLatA(), brg->GetLonA(), brg->GetLatB(), brg->GetLonB(), brg, SEL_SEG );

        GetOCPNCanvasWindow()->Refresh();

    }

    m_nfix = CalculateFix();
}
#endif

int ropeless_pi::CalculateFix( void )
{
    //  If there are two or more bearing lines stored, calculate the resulting fix
    //  Also, keep an array of points defining the fix for later rendering

    m_hat_array.Clear();

    if(m_brg_array.GetCount() < 2)
        return 0;

    double lat_acc = 0.;
    double lon_acc = 0.;
    int n_intersect = 0;
    for(size_t i=0 ; i < m_brg_array.GetCount()-1 ; i++){
        brg_line *a = m_brg_array.Item(i);
        brg_line *b = m_brg_array.Item(i+1);

        double lat, lon;
        if(a->getIntersect(b, &lat, &lon)){
            lat_acc += lat;
            lon_acc += lon;
            n_intersect ++;

            vector2D *pt = new vector2D(lon, lat);
            m_hat_array.Add(pt);
        }
    }

    if(m_brg_array.GetCount() > 2){
        brg_line *a = m_brg_array.Item(0);
        brg_line *b = m_brg_array.Item(m_brg_array.GetCount()-1);

        double lat, lon;
        if(a->getIntersect(b, &lat, &lon)){
            lat_acc += lat;
            lon_acc += lon;
            n_intersect ++;

            vector2D *pt = new vector2D(lon, lat);
            m_hat_array.Add(pt);
        }

    }

    //  Have at least one intersection,
    //  so calculate the fix as the average of all intersections
    if(n_intersect){
        m_fix_lat = lat_acc / n_intersect;
        m_fix_lon = lon_acc / n_intersect;
    }

    return (n_intersect);

}


void ropeless_pi::RenderFixHat( void )
{
#if 0
    if(!m_nfix)
        return;                 // no fix

    if(m_nfix == 1){            // two line fix

        if(m_hat_array.GetCount() == 1){
            vector2D *pt = m_hat_array.Item(0);
            wxPoint ab;
            GetCanvasPixLL(g_vp, &ab, pt->lat, pt->lon);

            int crad = 20;
            AlphaBlending( g_pdc, ab.x - crad, ab.y - crad, crad*2, crad *2, 3.0, m_FixHatColor, 250 );
        }
    }
    else if(m_nfix > 2){

        //      Get an array of wxPoints
//        printf("\n");
        wxPoint *pta = new wxPoint[m_nfix];
        for(unsigned int i=0 ; i < m_hat_array.GetCount() ; i++){
            vector2D *pt = m_hat_array.Item(i);
            wxPoint ab;
            GetCanvasPixLL(g_vp, &ab, pt->lat, pt->lon);
//            printf("%g %g %d %d\n", pt->lat, pt->lon, ab.x, ab.y);
            pta[i] = ab;
        }

        AlphaBlendingPoly( g_pdc, m_hat_array.GetCount(), pta, m_FixHatColor, 250 );

    }
#endif
}


void ropeless_pi::startSerial(const wxString &port)
{
    // verify the event handler
    if(!m_event_handler){
        m_event_handler = new PI_EventHandler(this);
    }

    if(port.Length()){
        if(m_serialThread){
        }

//        m_serialThread = new PI_OCP_DataStreamInput_Thread( this, m_event_handler, port, _T("4800"), DS_TYPE_INPUT);
        m_Thread_run_flag = 1;
        m_serialThread->Run();
    }
}

void ropeless_pi::stopSerial()
{
//    wxLogMessage( wxString::Format(_T("Closing NMEA Datastream %s"), m_portstring.c_str()) );

    //    Kill off the Secondary RX Thread if alive
    if(m_serialThread)
    {
        if(m_bsec_thread_active)              // Try to be sure thread object is still alive
        {
 //           wxLogMessage(_T("Stopping Secondary Thread"));

            m_Thread_run_flag = 0;
            int tsec = 10;
            while((m_Thread_run_flag >= 0) && (tsec--))
                wxSleep(1);

            wxString msg;
            if(m_Thread_run_flag < 0)
                msg.Printf(_T("Stopped in %d sec."), 10 - tsec);
            else
                msg.Printf(_T("Not Stopped after 10 sec."));
            wxLogMessage(msg);
        }

        m_serialThread = NULL;
        m_bsec_thread_active = false;
    }


}


void ropeless_pi::ShowPreferencesDialog( wxWindow* parent )
{
    TenderPrefsDialog *dialog = new TenderPrefsDialog( parent, wxID_ANY, _("Ropeless_pi Preferences"), wxPoint( 20, 20), wxDefaultSize, wxDEFAULT_DIALOG_STYLE );
    dialog->Fit();
    wxColour cl;
    GetGlobalColor(_T("DILG1"), &cl);
    dialog->SetBackgroundColour(cl);

    dialog->m_comboPort->SetValue(m_serialPort);
    dialog->m_wpComboPort->SetValue(m_trackedWP);

    dialog->m_comboIcon->SetValue(m_tenderIconType);

    wxString val;

    val.Printf(_T("%4d"), m_tenderGPS_x);
    dialog->m_pTenderGPSOffsetX->SetValue(val);
    val.Printf(_T("%4d"), m_tenderGPS_y);
    dialog->m_pTenderGPSOffsetY->SetValue(val);
    val.Printf(_T("%4d"), m_tenderLength);
    dialog->m_pTenderLength->SetValue(val);
    val.Printf(_T("%4d"), m_tenderWidth);
    dialog->m_pTenderWidth->SetValue(val);



    if(dialog->ShowModal() == wxID_OK)
    {
        m_serialPort = dialog->m_comboPort->GetValue();

        m_trackedWP = dialog->m_trackedPointName;
        m_trackedWPGUID = dialog->m_trackedPointGUID;

        m_tenderIconType = dialog->m_comboIcon->GetValue();

        long val;
        wxString str;
        str = dialog->m_pTenderGPSOffsetX->GetValue();
        if(str.ToLong(&val)) { m_tenderGPS_x = val; }

        str = dialog->m_pTenderGPSOffsetY->GetValue();
        if(str.ToLong(&val)) { m_tenderGPS_y = val; }

        str = dialog->m_pTenderLength->GetValue();
        if(str.ToLong(&val)) { m_tenderLength = val; }

        str = dialog->m_pTenderWidth->GetValue();
        if(str.ToLong(&val)) { m_tenderWidth = val; }

         SaveConfig();

         setTrackedWPSelect(m_trackedWPGUID);

    }
    delete dialog;
}

void ropeless_pi::ProcessTenderFix( void )
{
    // Maintain selectable point
    if(pos_valid){
        if(!m_tenderSelect){
            m_tenderSelect = m_select->AddSelectablePoint( gLat, gLon, this, SELTYPE_POINT_GENERIC, 0 );
        }
        else{
            m_select->ModifySelectablePoint( gLat, gLon, this, SELTYPE_POINT_GENERIC );
        }
    }

}

//-----------------------------------------------------------------------------------------------------------------
//
//      Bearing Line Class implementation
//
//-----------------------------------------------------------------------------------------------------------------

brg_line::brg_line(double bearing, BearingTypeEnum type)
{
    Init();

    m_bearing = bearing;
    m_type = type;
}

brg_line::brg_line(double bearing, BearingTypeEnum type, double lat_point, double lon_point, double length)
{
    Init();

    m_bearing = bearing;
    m_type = type;
    if(TRUE_BRG == type)
        m_bearing_true = bearing;
    else
        m_bearing_true = bearing - g_Var;

    m_latA = lat_point;
    m_lonA = lon_point;
    m_length = length;

    // shift point a just a bit (20% of length), to help make a cocked hat
    double adj_lata, adj_lona;
    double dy = -0.2 * m_length * 1852.0 * cos(m_bearing_true * PI / 180.);        // east/north in metres
    double dx = -0.2 * m_length * 1852.0 * sin(m_bearing_true * PI / 180.);
    fromSM_Plugin(dx, dy, m_latA, m_lonA, &adj_lata, &adj_lona);

    m_latA = adj_lata;
    m_lonA = adj_lona;

    CalcPointB();
}

brg_line::~brg_line()
{
}


void brg_line::Init(void)
{
    //  Set some innocent initial values
    m_bearing = 0;
    m_bearing_true = 0;
    m_type = TRUE_BRG;
    m_latA = m_latB = m_lonA = m_lonB = 0;
    m_length = 100;

    m_color = wxColour(0,0,0);
    m_width = 4;

    m_color_text = wxColour(0,0,0);

    m_Font = wxTheFontList->FindOrCreateFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    m_create_time = wxDateTime::Now();
    m_InfoBoxColor = wxTheColourDatabase->Find(_T("LIME GREEN"));

}

bool brg_line::getIntersect(brg_line* b, double* lat, double* lon)
{
    double ilat, ilon;

    MyFlPoint p1; p1.x = this->m_lonA; p1.y = this->m_latA;
    MyFlPoint p2; p2.x = this->m_lonB; p2.y = this->m_latB;
    MyFlPoint p3; p3.x = b->m_lonA; p3.y = b->m_latA;
    MyFlPoint p4; p4.x = b->m_lonB; p4.y = b->m_latB;


    bool ret = LineIntersect( p1, p2, p3, p4, &ilon, &ilat);

    if(lat)
        *lat = ilat;
    if(lon)
        *lon = ilon;

    return ret;

}



void brg_line::CalcPointB(void)
{
    //  We calculate the other endpoint using simple mercator projection

    double dy = m_length * 1852.0 * cos(m_bearing_true * PI / 180.);        // east/north in metres
    double dx = m_length * 1852.0 * sin(m_bearing_true * PI / 180.);

    fromSM_Plugin(dx, dy, m_latA, m_lonA, &m_latB, &m_lonB);

}

void brg_line::CalcLength(void)
{
    // We re-calculate the length from the two endpoints
    double dx, dy;
    toSM_Plugin(m_latA, m_lonA, m_latB, m_lonB, &dx, &dy);
    double distance = sqrt( (dx * dx) + (dy * dy));

    m_length = distance / 1852.;
}

void brg_line::DrawInfoBox( void )
{
#if 0
    //  Calculate the points
    wxPoint ab;
    GetCanvasPixLL(g_vp, &ab, m_latA, m_lonA);

    wxPoint cd;
    GetCanvasPixLL(g_vp, &cd, m_latB, m_lonB);

    //  Draw a blank, semi-transparent box
    // Where to draw the info box, by default?
    //  Centered on the brg_line
    int box_width = 200;
    int box_height = 50;



    //  Create the text info string
    g_pdc->SetFont(*m_Font);

    //  Use a sample string to gather some text font parameters
    wxString info;
    int sx, sy;
    int ypitch;
    int max_info_text_length = 0;
    int xmargin = 5;    // margin for text in the box

    wxString sample = _T("W");
    g_pdc->GetTextExtent(sample, &sx, &sy);
    ypitch = sy;    // line pitch

    info.Printf(_T("Bearing: %g\n"), m_bearing_true);
    g_pdc->GetTextExtent(info, &sx, &sy);
    max_info_text_length = wxMax(max_info_text_length, sx);
    int nlines = 1;

    //  Now add some more lines
    wxString info2 = _T("Capture time: ");
    info2 << m_create_time.FormatDate() <<_T("  ") << m_create_time.FormatTime();
    g_pdc->GetTextExtent(info2, &sx, &sy);
    max_info_text_length = wxMax(max_info_text_length, sx);
    info  << info2;
    nlines++;

    // Draw the blank box

    // Size...
    box_width = max_info_text_length + (xmargin * 2);
    box_height = nlines * (ypitch + 4);

    //Position...
    int xp = ((ab.x + cd.x)/2) ;
    int yp = ((ab.y + cd.y)/2) ;

    double angle = 0.;
    double distance = box_width * .75;

    double dx = 0;
    double dy = 0;
//    double lx = 0;
//    double ly = 0;

    //  Position of info box depends on orientation of the bearing line.
    if((m_bearing_true >0 ) && (m_bearing_true < 90)){
        angle = m_bearing_true - 90.;
        angle = angle * PI / 180.;
        dx = distance * cos(angle);
        dy = -distance * sin(angle);
//        lx = 0;
//        ly = 0;
    }

    else if((m_bearing_true > 90 ) && (m_bearing_true < 180)){
      angle = m_bearing_true - 90.;
      angle = angle * PI / 180.;
      dx = (distance * sin(angle));
      dy = -distance * cos(angle);
//      lx = 0;
//      ly = 0;
    }

  else if((m_bearing_true > 180 ) && (m_bearing_true < 270)){
      angle = m_bearing_true - 90.;
      angle = angle * PI / 180.;
      dx = -(distance * sin(angle)) - box_width;
      dy = distance * cos(angle);
//      lx = box_width;
//      ly = 0;
  }

  else if((m_bearing_true > 270 ) && (m_bearing_true < 360)){
      angle = m_bearing_true - 90.;
      angle = angle * PI / 180.;
      dx = (distance * sin(angle)) - box_width;
      dy = (-distance * cos(angle));
//      lx = box_width;
//      ly = 0;
  }

    int box_xp = xp + dx;
    int box_yp = yp + dy;


    AlphaBlending( g_pdc, box_xp, box_yp, box_width, box_height, 6.0, m_InfoBoxColor, 127 );

    //  Leader line
//    RenderLine(xp, yp, box_xp + lx, box_yp + ly, m_InfoBoxColor, 4);

    //  Now draw the text

    g_pdc->DrawLabel( info, wxRect( box_xp + xmargin, box_yp, box_width, box_height ),
                      wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL);


    if(g_pdc) {
        wxPen ppbrg ( m_color, m_width );

#if wxUSE_GRAPHICS_CONTEXT
        g_gdc->SetPen(ppbrg);
#endif

        g_pdc->SetPen(ppbrg);
    } else { /* opengl */

#ifdef ocpnUSE_GL
///glColor4ub(m_color.Red(), m_color.Green(), m_color.Blue(),  255/*m_color.Alpha()*/);
#endif
    }



     if(g_pdc) {
#if wxUSE_GRAPHICS_CONTEXT
//        g_gdc->StrokeLine(ab.x, ab.y, cd.x, cd.y);
#else
//        g_dc->DrawLine(ab.x, ab.y, cd.x, cd.y);
#endif
     } else { /* opengl */
#ifdef ocpnUSE_GL
//        pof->DrawGLLine(ab.x, ab.y, cd.x, cd.y, 2);
#endif
     }
#endif
}



void brg_line::DrawInfoAligned( void )
{
#if 0
    //  Calculate the points
    wxPoint ab;
    GetCanvasPixLL(g_vp, &ab, m_latA, m_lonA);

    wxPoint cd;
    GetCanvasPixLL(g_vp, &cd, m_latB, m_lonB);

    //  Create the text info string

    wxString info;
    info.Printf(_T("Bearing: %g"), m_bearing_true);
    wxString degree = wxString::Format(_T("%cT"), 0x00B0); //_T("°");
    info += degree;
    wxString info2 = _T("  Ident: ");
    info2 += m_Ident;

    info += info2;

    //  Use a screen DC to calulate the drawing location/size
    wxScreenDC sdc;
    sdc.SetFont(*m_Font);

    int sx, sy;
    sdc.GetTextExtent(info, &sx, &sy);

    int offset = 5;
    double angle = m_bearing_true * PI / 180.;

    int off_x = offset * cos(angle);
    int off_y = offset * sin(angle);

    int ctr_x = -(sx/2) * sin(angle);
    int ctr_y = (sx/2) * cos(angle);

    if(m_bearing_true < 180.){
        int xp, yp;
        xp = ctr_x + off_x + (ab.x + cd.x)/2;
        yp = ctr_y + off_y + (ab.y + cd.y)/2;

        RenderText( g_pdc, g_gdc, info, m_Font, m_color_text, xp, yp, 90. - m_bearing_true);
    }
    else{
        int xp, yp;
        xp = -ctr_x - off_x + (ab.x + cd.x)/2;
        yp = -ctr_y - off_y + (ab.y + cd.y)/2;

        RenderText( g_pdc, g_gdc, info, m_Font, m_color_text, xp, yp, 90. - m_bearing_true + 180.);
    }
#endif
}

void brg_line::Draw( void )
{
    //  Calculate the points
    wxPoint ab;
    GetCanvasPixLL(g_vp, &ab, m_latA, m_lonA);

    wxPoint cd;
    GetCanvasPixLL(g_vp, &cd, m_latB, m_lonB);

    //  Adjust the rendering width, to make it 100 metres wide
    double dwidth = 100 * g_ovp.view_scale_ppm;

    dwidth = wxMin(4, dwidth);
    dwidth = wxMax(2, dwidth);

//    printf("%g\n", dwidth);
    //RenderLine(g_pdc, g_gdc, ab.x, ab.y, cd.x, cd.y, m_color, dwidth);

}




//      Event Handler implementation
BEGIN_EVENT_TABLE ( PI_EventHandler, wxEvtHandler )
EVT_TIMER ( ROLLOVER_TIMER, PI_EventHandler::OnTimerEvent )
EVT_TIMER ( HEAD_DOG_TIMER, PI_EventHandler::OnTimerEvent )

END_EVENT_TABLE()



PI_EventHandler::PI_EventHandler(ropeless_pi * parent)
{
    m_parent = parent;
    Connect( wxEVT_PI_OCPN_DATASTREAM, (wxObjectEventFunction) (wxEventFunction) &PI_EventHandler::OnEvtOCPN_NMEA );

}

PI_EventHandler::~PI_EventHandler()
{
    Disconnect( wxEVT_PI_OCPN_DATASTREAM, (wxObjectEventFunction) (wxEventFunction) &PI_EventHandler::OnEvtOCPN_NMEA );

}


void PI_EventHandler::OnTimerEvent(wxTimerEvent& event)
{
    m_parent->ProcessTimerEvent( event );
}

void PI_EventHandler::PopupMenuHandler(wxCommandEvent& event )
{
    m_parent->PopupMenuHandler( event );

}

void PI_EventHandler::OnEvtOCPN_NMEA( PI_OCPN_DataStreamEvent& event )
{
    wxString str_buf = event.ProcessNMEA4Tags();

    g_NMEA0183 << str_buf;
    if( g_NMEA0183.PreParse() )
    {
        if( g_NMEA0183.LastSentenceIDReceived == _T("RMC") )
        {
            if( g_NMEA0183.Parse() )
            {
                if( g_NMEA0183.Rmc.IsDataValid == NTrue )
                {
                    ll_valid = true;    // tentatively

                    if( !wxIsNaN(g_NMEA0183.Rmc.Position.Latitude.Latitude) )
                    {
                        double llt = g_NMEA0183.Rmc.Position.Latitude.Latitude;
                        int lat_deg_int = (int) ( llt / 100 );
                        double lat_deg = lat_deg_int;
                        double lat_min = llt - ( lat_deg * 100 );
                        gLat = lat_deg + ( lat_min / 60. );
                        if( g_NMEA0183.Rmc.Position.Latitude.Northing == South ) gLat = -gLat;
                    }
                    else
                        ll_valid = false;

                    if( !wxIsNaN(g_NMEA0183.Rmc.Position.Longitude.Longitude) )
                    {
                        double lln = g_NMEA0183.Rmc.Position.Longitude.Longitude;
                        int lon_deg_int = (int) ( lln / 100 );
                        double lon_deg = lon_deg_int;
                        double lon_min = lln - ( lon_deg * 100 );
                        gLon = lon_deg + ( lon_min / 60. );
                        if( g_NMEA0183.Rmc.Position.Longitude.Easting == West )
                            gLon = -gLon;
                    }
                    else
                        ll_valid = false;

                    gSog = g_NMEA0183.Rmc.SpeedOverGroundKnots;
                    gCog = g_NMEA0183.Rmc.TrackMadeGoodDegreesTrue;

                     if( !wxIsNaN(g_NMEA0183.Rmc.MagneticVariation) )
                     {
                         if( g_NMEA0183.Rmc.MagneticVariationDirection == East )
                             gVar = g_NMEA0183.Rmc.MagneticVariation;
                         else
                             if( g_NMEA0183.Rmc.MagneticVariationDirection == West )
                                 gVar = -g_NMEA0183.Rmc.MagneticVariation;

//                             g_bVAR_Rx = true;
//                         gVAR_Watchdog = gps_watchdog_timeout_ticks;
                     }

 //                   sfixtime = g_NMEA0183.Rmc.UTCTime;

//                     if(ll_valid )
//                     {
//                         gGPS_Watchdog = gps_watchdog_timeout_ticks;
//                         wxDateTime now = wxDateTime::Now();
//                         m_fixtime = now.GetTicks();
//                     }
                    pos_valid = ll_valid;
                    gGPS_Watchdog = 0;          // feed the dog
                    m_parent->ProcessTenderFix();




                }
                else
                    pos_valid = false;
            }
        }
        else
            if( g_NMEA0183.LastSentenceIDReceived == _T("HDT") )
            {
                if( g_NMEA0183.Parse() )
                {
                    gHdt = g_NMEA0183.Hdt.DegreesTrue;
                    if( !wxIsNaN(g_NMEA0183.Hdt.DegreesTrue) )
                    {
                      //  g_bHDT_Rx = true;
                        gHDT_Watchdog = 0;
                    }
                }
            }

        else
              if( g_NMEA0183.LastSentenceIDReceived == _T("HDM") )
              {
                  if( g_NMEA0183.Parse() )
                  {
                      gHdm = g_NMEA0183.Hdm.DegreesMagnetic;
                      //if( !wxIsNaN(g_NMEA0183.Hdm.DegreesMagnetic) )
                        //  gHDx_Watchdog = gps_watchdog_timeout_ticks;
                  }
              }

    }

}



wxArrayString *EnumerateSerialPorts( void );

BEGIN_EVENT_TABLE( TenderPrefsDialog, wxDialog )
EVT_BUTTON( wxID_OK, TenderPrefsDialog::OnTenderPrefsOkClick )
END_EVENT_TABLE()

TenderPrefsDialog::TenderPrefsDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{

        this->SetSizeHints( wxDefaultSize, wxDefaultSize );

        wxBoxSizer* bSizer2;
        bSizer2 = new wxBoxSizer( wxVERTICAL );

        wxStaticBoxSizer* sbSizerP = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tender GPS Serial Port") ), wxVERTICAL );

        m_comboPort = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
        sbSizerP->Add(m_comboPort, 0, wxEXPAND | wxTOP, 5);

        bSizer2->Add( sbSizerP, 0, wxALL|wxEXPAND, 5 );

        m_pSerialArray = NULL;
        m_pSerialArray = EnumerateSerialPorts();

        if (m_pSerialArray) {
            m_comboPort->Clear();
            for (size_t i = 0; i < m_pSerialArray->Count(); i++) {
                m_comboPort->Append(m_pSerialArray->Item(i));
            }
        }

        // Get the global waypoint list

        wxStaticBoxSizer* sbSizerwP = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tracked Waypoint (by Name)") ), wxVERTICAL );

        m_wpComboPort = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
        sbSizerwP->Add(m_wpComboPort, 0, wxEXPAND | wxTOP, 5);

        bSizer2->Add( sbSizerwP, 0, wxALL|wxEXPAND, 5 );

        wxArrayString guidArray = GetWaypointGUIDArray();

        m_wpComboPort->Clear();
        for(unsigned int i=0 ; i < guidArray.GetCount() ; i++){
            wxString name = getWaypointName( guidArray[i] );
            if(name.Length())
                m_wpComboPort->Append(name);
        }


        // Icon type
        wxStaticBoxSizer* sbSizerIcon = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tender Icon") ), wxVERTICAL );

        m_comboIcon = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
        sbSizerIcon->Add(m_comboIcon, 0, wxEXPAND | wxTOP, 5);

        bSizer2->Add( sbSizerIcon, 0, wxALL|wxEXPAND, 5 );


        m_comboIcon->Clear();
        for (size_t i = 0; i < g_iconTypeArray.Count(); i++) {
            m_comboIcon->Append(g_iconTypeArray.Item(i));
        }

        // Icon parameters
        wxStaticBoxSizer* sbSizerDimensions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tender Dimensions") ), wxVERTICAL );
        bSizer2->Add( sbSizerDimensions, 1, wxALL|wxEXPAND, 5 );

        wxFlexGridSizer *realSizes = new wxFlexGridSizer(5, 2, 4, 4);
        realSizes->AddGrowableCol(1);

        sbSizerDimensions->Add(realSizes, 0, wxEXPAND | wxLEFT, 30);

        realSizes->Add( new wxStaticText(this, wxID_ANY, _("Length Over All (m)")), 1, wxALIGN_LEFT);
        m_pTenderLength = new wxTextCtrl(this, 1);
        realSizes->Add(m_pTenderLength, 1, wxALIGN_RIGHT | wxALL, 4);

        realSizes->Add( new wxStaticText(this, wxID_ANY, _("Width Over All (m)")), 1, wxALIGN_LEFT);
        m_pTenderWidth = new wxTextCtrl(this, wxID_ANY);
        realSizes->Add(m_pTenderWidth, 1, wxALIGN_RIGHT | wxALL, 4);

        realSizes->Add( new wxStaticText(this, wxID_ANY, _("GPS Offset from Bow (m)")),1, wxALIGN_LEFT);
        m_pTenderGPSOffsetY = new wxTextCtrl(this, wxID_ANY);
        realSizes->Add(m_pTenderGPSOffsetY, 1, wxALIGN_RIGHT | wxALL, 4);

        realSizes->Add(new wxStaticText(this, wxID_ANY, _("GPS Offset from Midship (m)")), 1, wxALIGN_LEFT);
        m_pTenderGPSOffsetX = new wxTextCtrl(this, wxID_ANY);
        realSizes->Add(m_pTenderGPSOffsetX, 1, wxALIGN_RIGHT | wxALL, 4);


 /*
        wxString m_rbViewTypeChoices[] = { _("Extended"), _("Variation only") };
        int m_rbViewTypeNChoices = sizeof( m_rbViewTypeChoices ) / sizeof( wxString );
        m_rbViewType = new wxRadioBox( this, wxID_ANY, _("View"), wxDefaultPosition, wxDefaultSize, m_rbViewTypeNChoices, m_rbViewTypeChoices, 2, wxRA_SPECIFY_COLS );
        m_rbViewType->SetSelection( 1 );
        bSizer2->Add( m_rbViewType, 0, wxALL|wxEXPAND, 5 );

        m_cbShowPlotOptions = new wxCheckBox( this, wxID_ANY, _("Show Plot Options"), wxDefaultPosition, wxDefaultSize, 0 );
        bSizer2->Add( m_cbShowPlotOptions, 0, wxALL, 5 );

        m_cbShowAtCursor = new wxCheckBox( this, wxID_ANY, _("Show also data at cursor position"), wxDefaultPosition, wxDefaultSize, 0 );
        bSizer2->Add( m_cbShowAtCursor, 0, wxALL, 5 );

        m_cbShowIcon = new wxCheckBox( this, wxID_ANY, _("Show toolbar icon"), wxDefaultPosition, wxDefaultSize, 0 );
        bSizer2->Add( m_cbShowIcon, 0, wxALL, 5 );

        m_cbLiveIcon = new wxCheckBox( this, wxID_ANY, _("Show data in toolbar icon"), wxDefaultPosition, wxDefaultSize, 0 );
        bSizer2->Add( m_cbLiveIcon, 0, wxALL, 5 );

        wxStaticBoxSizer* sbSizer4;
        sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Window transparency") ), wxVERTICAL );

        m_sOpacity = new wxSlider( this, wxID_ANY, 255, 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_INVERSE );
        sbSizer4->Add( m_sOpacity, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


        bSizer2->Add( sbSizer4, 1, wxALL|wxEXPAND, 5 );
*/

        m_sdbSizer1 = new wxStdDialogButtonSizer();
        m_sdbSizer1OK = new wxButton( this, wxID_OK );
        m_sdbSizer1->AddButton( m_sdbSizer1OK );
        m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
        m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
        m_sdbSizer1->Realize();

        bSizer2->Add( m_sdbSizer1, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


        this->SetSizer( bSizer2 );
        this->Layout();
        bSizer2->Fit( this );

        this->Centre( wxBOTH );
}

TenderPrefsDialog::~TenderPrefsDialog()
{
        delete m_pSerialArray;
}

void TenderPrefsDialog::OnTenderPrefsOkClick(wxCommandEvent& event)
{
    m_trackedPointName = m_wpComboPort->GetValue();

    wxArrayString guidArray = GetWaypointGUIDArray();
    for(unsigned int i=0 ; i < guidArray.GetCount() ; i++){
        wxString name = getWaypointName( guidArray[i] );
        if(name.Length()){
            if(name.IsSameAs(m_trackedPointName)){
                m_trackedPointGUID = guidArray[i];
                break;
            }
        }
    }

    EndModal( wxID_OK );

}




/*
 *     Enumerate all the serial ports on the system
 *
 *     wxArrayString *EnumerateSerialPorts(void)

 *     Very system specific, unavoidably.
 */

#if defined(__UNIX__) && !defined(__OCPN__ANDROID__) && !defined(__WXOSX__)
extern "C" int wait(int *);                     // POSIX wait() for process

#include <termios.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

#endif

// ****************************************
// Fulup devices selection with scandir
// ****************************************

// reserve 4 pattern for plugins
char* devPatern[] = {
  NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL, (char*)-1};


// This function allow external plugin to search for a special device name
// ------------------------------------------------------------------------
int paternAdd (const char* patern) {
  int ind;

  // snan table for a free slot inside devpatern table
  for (ind=0; devPatern[ind] != (char*)-1; ind++)
       if (devPatern[ind] == NULL) break;

  // table if full
  if  (devPatern [ind] == (char*) -1) return -1;

  // store a copy of the patern in case calling routine had it on its stack
  devPatern [ind]  = strdup (patern);
  return 0;
}


#if defined(__UNIX__) && !defined(__OCPN__ANDROID__) && !defined(__WXOSX__)
// This filter verify is device is withing searched patern and verify it is openable
// -----------------------------------------------------------------------------------
int paternFilter (const struct dirent * dir) {
 char* res = NULL;
 char  devname [512];
 int   fd, ind;

  // search if devname fits with searched paterns
  for (ind=0; devPatern [ind] != (char*)-1; ind++) {
     if (devPatern [ind] != NULL) res=(char*)strcasestr(dir->d_name,devPatern [ind]);
     if (res != NULL) break;
  }

  // File does not fit researched patern
  if (res == NULL) return 0;

  // Check if we may open this file
  snprintf (devname, sizeof (devname), "/dev/%s", dir->d_name);
  fd = open(devname, O_RDWR|O_NDELAY|O_NOCTTY);

  // device name is pointing to a real device
  if(fd > 0) {
    close (fd);
    return 1;
  }

  // file is not valid
  perror (devname);
  return 0;
}

int isTTYreal(const char *dev)
{
    struct serial_struct serinfo;
    int ret = 0;

    int fd = open(dev, O_RDWR | O_NONBLOCK | O_NOCTTY);

    // device name is pointing to a real device
    if(fd >= 0) {
        if (ioctl(fd, TIOCGSERIAL, &serinfo)==0) {
            // If device type is no PORT_UNKNOWN we accept the port
            if (serinfo.type != PORT_UNKNOWN)
                ret = 1;
        }
        close (fd);
    }

    return ret;
}


#endif

#ifdef __MINGW32__ // do I need this because of mingw, or because I am running mingw under wine?
# ifndef GUID_CLASS_COMPORT
DEFINE_GUID(GUID_CLASS_COMPORT, 0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);
# endif
#endif

wxArrayString *EnumerateSerialPorts( void )
{
    wxArrayString *preturn = new wxArrayString;

#if defined(__UNIX__) && !defined(__OCPN__ANDROID__) && !defined(__WXOSX__)

    //Initialize the pattern table
    if( devPatern[0] == NULL ) {
        paternAdd ( "ttyUSB" );
        paternAdd ( "ttyACM" );
        paternAdd ( "ttyGPS" );
        paternAdd ( "refcom" );
    }

 //  Looking for user privilege openable devices in /dev
 //  Fulup use scandir to improve user experience and support new generation of AIS devices.

      wxString sdev;
      int ind, fcount;
      struct dirent **filelist = {0};

      // scan directory filter is applied automatically by this call
      fcount = scandir("/dev", &filelist, paternFilter, alphasort);

      for(ind = 0; ind < fcount; ind++)  {
       wxString sdev (filelist[ind]->d_name, wxConvUTF8);
       sdev.Prepend (_T("/dev/"));

       preturn->Add (sdev);
       free(filelist[ind]);
      }

      free(filelist);

//        We try to add a few more, arbitrarily, for those systems that have fixed, traditional COM ports

    if( isTTYreal("/dev/ttyS0") )
        preturn->Add( _T("/dev/ttyS0") );

    if( isTTYreal("/dev/ttyS1") )
        preturn->Add( _T("/dev/ttyS1") );


#endif

#ifdef PROBE_PORTS__WITH_HELPER

    /*
     *     For modern Linux/(Posix??) systems, we may use
     *     the system files /proc/tty/driver/serial
     *     and /proc/tty/driver/usbserial to identify
     *     available serial ports.
     *     A complicating factor is that most (all??) linux
     *     systems require root privileges to access these files.
     *     We will use a helper program method here, despite implied vulnerability.
     */

    char buf[256]; // enough to hold one line from serial devices list
    char left_digit;
    char right_digit;
    int port_num;
    FILE *f;

    pid_t pID = vfork();

    if (pID == 0)// child
    {
//    Temporarily gain root privileges
        seteuid(file_user_id);

//  Execute the helper program
        execlp("ocpnhelper", "ocpnhelper", "-SB", NULL);

//  Return to user privileges
        seteuid(user_user_id);

        wxLogMessage(_T("Warning: ocpnhelper failed...."));
        _exit(0);// If exec fails then exit forked process.
    }

    wait(NULL);                  // for the child to quit

//    Read and parse the files

    /*
     * see if we have any traditional ttySx ports available
     */
    f = fopen("/var/tmp/serial", "r");

    if (f != NULL)
    {
        wxLogMessage(_T("Parsing copy of /proc/tty/driver/serial..."));

        /* read in each line of the file */
        while(fgets(buf, sizeof(buf), f) != NULL)
        {
            wxString sm(buf, wxConvUTF8);
            sm.Prepend(_T("   "));
            sm.Replace(_T("\n"), _T(" "));
            wxLogMessage(sm);

            /* if the line doesn't start with a number get the next line */
            if (buf[0] < '0' || buf[0] > '9')
            continue;

            /*
             * convert digits to an int
             */
            left_digit = buf[0];
            right_digit = buf[1];
            if (right_digit < '0' || right_digit > '9')
            port_num = left_digit - '0';
            else
            port_num = (left_digit - '0') * 10 + right_digit - '0';

            /* skip if "unknown" in the string */
            if (strstr(buf, "unknown") != NULL)
            continue;

            /* upper limit of 15 */
            if (port_num > 15)
            continue;

            /* create string from port_num  */

            wxString s;
            s.Printf(_T("/dev/ttyS%d"), port_num);

            /*  add to the output array  */
            preturn->Add(wxString(s));

        }

        fclose(f);
    }

    /*
     * Same for USB ports
     */
    f = fopen("/var/tmp/usbserial", "r");

    if (f != NULL)
    {
        wxLogMessage(_T("Parsing copy of /proc/tty/driver/usbserial..."));

        /* read in each line of the file */
        while(fgets(buf, sizeof(buf), f) != NULL)
        {

            wxString sm(buf, wxConvUTF8);
            sm.Prepend(_T("   "));
            sm.Replace(_T("\n"), _T(" "));
            wxLogMessage(sm);

            /* if the line doesn't start with a number get the next line */
            if (buf[0] < '0' || buf[0] > '9')
            continue;

            /*
             * convert digits to an int
             */
            left_digit = buf[0];
            right_digit = buf[1];
            if (right_digit < '0' || right_digit > '9')
            port_num = left_digit - '0';
            else
            port_num = (left_digit - '0') * 10 + right_digit - '0';

            /* skip if "unknown" in the string */
            if (strstr(buf, "unknown") != NULL)
            continue;

            /* upper limit of 15 */
            if (port_num > 15)
            continue;

            /* create string from port_num  */

            wxString s;
            s.Printf(_T("/dev/ttyUSB%d"), port_num);

            /*  add to the output array  */
            preturn->Add(wxString(s));

        }

        fclose(f);
    }

    //    As a fallback, in case seteuid doesn't work....
    //    provide some defaults
    //    This is currently the case for GTK+, which
    //    refuses to run suid.  sigh...

    if(preturn->IsEmpty())
    {
        preturn->Add( _T("/dev/ttyS0"));
        preturn->Add( _T("/dev/ttyS1"));
        preturn->Add( _T("/dev/ttyUSB0"));
        preturn->Add( _T("/dev/ttyUSB1"));
        preturn->Add( _T("/dev/ttyACM0"));
        preturn->Add( _T("/dev/ttyACM1"));
    }

//    Clean up the temporary files created by helper.
    pid_t cpID = vfork();

    if (cpID == 0)// child
    {
//    Temporarily gain root privileges
        seteuid(file_user_id);

//  Execute the helper program
        execlp("ocpnhelper", "ocpnhelper", "-U", NULL);

//  Return to user privileges
        seteuid(user_user_id);
        _exit(0);// If exec fails then exit forked process.
    }

#endif      // __WXGTK__
#ifdef __WXOSX__
#include "macutils.h"
    char* paPortNames[MAX_SERIAL_PORTS];
    int iPortNameCount;

    memset(paPortNames,0x00,sizeof(paPortNames));
    iPortNameCount = FindSerialPortNames(&paPortNames[0],MAX_SERIAL_PORTS);
    for (int iPortIndex=0; iPortIndex<iPortNameCount; iPortIndex++)
    {
        wxString sm(paPortNames[iPortIndex], wxConvUTF8);
        preturn->Add(sm);
        free(paPortNames[iPortIndex]);
    }
#endif      //__WXOSX__
#ifdef __WXMSW__
    /*************************************************************************
     * Windows provides no system level enumeration of available serial ports
     * There are several ways of doing this.
     *
     *************************************************************************/

#include <windows.h>

    //    Method 1:  Use GetDefaultCommConfig()
    // Try first {g_nCOMPortCheck} possible COM ports, check for a default configuration
    //  This method will not find some Bluetooth SPP ports
    for( int i = 1; i < 8; i++ ) {
        wxString s;
        s.Printf( _T("COM%d"), i );

        COMMCONFIG cc;
        DWORD dwSize = sizeof(COMMCONFIG);
        if( GetDefaultCommConfig( s.fn_str(), &cc, &dwSize ) )
            preturn->Add( wxString( s ) );
    }

#if 0
    // Method 2:  Use FileOpen()
    // Try all 255 possible COM ports, check to see if it can be opened, or if
    // not, that an expected error is returned.

    BOOL bFound;
    for (int j=1; j<256; j++)
    {
        char s[20];
        sprintf(s, "\\\\.\\COM%d", j);

        // Open the port tentatively
        BOOL bSuccess = FALSE;
        HANDLE hComm = ::CreateFile(s, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

        //  Check for the error returns that indicate a port is there, but not currently useable
        if (hComm == INVALID_HANDLE_VALUE)
        {
            DWORD dwError = GetLastError();

            if (dwError == ERROR_ACCESS_DENIED ||
                    dwError == ERROR_GEN_FAILURE ||
                    dwError == ERROR_SHARING_VIOLATION ||
                    dwError == ERROR_SEM_TIMEOUT)
            bFound = TRUE;
        }
        else
        {
            bFound = TRUE;
            CloseHandle(hComm);
        }

        if (bFound)
        preturn->Add(wxString(s));
    }
#endif

    // Method 3:  WDM-Setupapi
    //  This method may not find XPort virtual ports,
    //  but does find Bluetooth SPP ports

    GUID *guidDev = (GUID*) &GUID_CLASS_COMPORT;

    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;

    hDevInfo = SetupDiGetClassDevs( guidDev,
                                     NULL,
                                     NULL,
                                     DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );

    if(hDevInfo != INVALID_HANDLE_VALUE) {

        BOOL bOk = TRUE;
        SP_DEVICE_INTERFACE_DATA ifcData;

        ifcData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        for (DWORD ii=0; bOk; ii++) {
            bOk = SetupDiEnumDeviceInterfaces(hDevInfo, NULL, guidDev, ii, &ifcData);
            if (bOk) {
            // Got a device. Get the details.

                SP_DEVINFO_DATA devdata = {sizeof(SP_DEVINFO_DATA)};
                bOk = SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                                      &ifcData, NULL, 0, NULL, &devdata);

                //      We really only need devdata
                if( !bOk ) {
                    if( GetLastError() == 122)  //ERROR_INSUFFICIENT_BUFFER, OK in this case
                        bOk = true;
                }
//#if 0
                //      We could get friendly name and/or description here
                TCHAR fname[256] = {0};
                TCHAR desc[256] ={0};
                if (bOk) {
                    BOOL bSuccess = SetupDiGetDeviceRegistryProperty(
                        hDevInfo, &devdata, SPDRP_FRIENDLYNAME, NULL,
                        (PBYTE)fname, sizeof(fname), NULL);

                    bSuccess = bSuccess && SetupDiGetDeviceRegistryProperty(
                        hDevInfo, &devdata, SPDRP_DEVICEDESC, NULL,
                        (PBYTE)desc, sizeof(desc), NULL);
                }
//#endif
                //  Get the "COMn string from the registry key
                if(bOk) {
                    bool bFoundCom = false;
                    TCHAR dname[256];
                    HKEY hDeviceRegistryKey = SetupDiOpenDevRegKey(hDevInfo, &devdata,
                                                                   DICS_FLAG_GLOBAL, 0,
                                                                   DIREG_DEV, KEY_QUERY_VALUE);
                    if(INVALID_HANDLE_VALUE != hDeviceRegistryKey) {
                            DWORD RegKeyType;
                            wchar_t    wport[80];
                            LPCWSTR cstr = wport;
                            MultiByteToWideChar( 0, 0, "PortName", -1, wport, 80);
                            DWORD len = sizeof(dname);

                            int result = RegQueryValueEx(hDeviceRegistryKey, cstr,
                                                        0, &RegKeyType, (PBYTE)dname, &len );
                            if( result == 0 )
                                bFoundCom = true;
                    }

                    if( bFoundCom ) {
                        wxString port( dname, wxConvUTF8 );

                        //      If the port has already been found, remove the prior entry
                        //      in favor of this entry, which will have descriptive information appended
                        for( unsigned int n=0 ; n < preturn->GetCount() ; n++ ) {
                            if((preturn->Item(n)).IsSameAs(port)){
                                preturn->RemoveAt( n );
                                break;
                            }
                        }
                        wxString desc_name( desc, wxConvUTF8 );         // append "description"
                        port += _T(" ");
                        port += desc_name;

                        preturn->Add( port );
                    }
                }
            }
        }//for
    }// if

#endif      //__WXMSW__
    return preturn;
}






