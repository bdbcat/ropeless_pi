/******************************************************************************
 * $Id:
 *
 * Project:  OpenCPN
 * Purpose:
 * Author:   dsr
 *
 ***************************************************************************
 *   Copyright (C) 2016 by David S. Register                               *
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

#ifndef _ROPELESSPI_H_
#define _ROPELESSPI_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

//#define     PLUGIN_VERSION_MAJOR    0
//#define     PLUGIN_VERSION_MINOR    6

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    12

#include <vector>

#include "graphics.h"

#include <wx/notebook.h>
#include <wx/fileconf.h>
#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/spinctrl.h>
#include <wx/aui/aui.h>
#include <wx/fontpicker.h>
//#include "/home/dsr/Projects/opencpn_sf/opencpn/include/ocpn_plugin.h"
#include "ocpn_plugin.h"
#include "ODdc.h"
#include "pugixml.hpp"

#include "nmea0183/nmea0183.h"
#include "PI_RolloverWin.h"
#include "TexFont.h"
#include "vector2d.h"
#include "OCPN_DataStreamEvent.h"
#include <deque>
#include <wx/socket.h>

#define EPL_TOOL_POSITION -1          // Request default positioning of toolbar tool

#define gps_watchdog_timeout_ticks  10

//    Constants
#ifndef PI
#define PI        3.1415926535897931160E0      /* pi */
#endif

#define SEL_POINT_A     0
#define SEL_POINT_B     1
#define SEL_SEG         2

//      Menu items
#define ID_EPL_DELETE           8867
#define ID_EPL_XMIT             8868
#define ID_TPR_RELEASE          8869
#define ID_TPR_DELETE           8870
#define ID_TPR_ID               8871
#define ID_TPR_MANUAL_RELEASE    8872

//      Message IDs
#define SIM_TIMER 5003
#define HISTORY_FADE_SECS    10
#define ID_PLAY_SIM 5058
#define ID_STOP_SIM 5059
#define ID_TRANSPONDER_LIST 5060

enum {
  tlICON = 0,
  tlIDENT,
  tlTIMESTAMP,
  tlRELEASE_STATUS,
  tlDISTANCE,
  tlPINGS,
  tlDEPTH,
  tlTEMP
};  // Transponder list Columns;

//----------------------------------------------------------------------------------------------------------
//    Forward declarations
//----------------------------------------------------------------------------------------------------------
class Select;
class SelectItem;
class PI_EventHandler;
class PI_OCP_DataStreamInput_Thread;
class RopelessDialog;
class OCPNListCtrl;

WX_DECLARE_OBJARRAY(vector2D *, ArrayOf2DPoints);

//void AlphaBlending( wxDC *pdc, int x, int y, int size_x, int size_y, float radius, wxColour color,
//                    unsigned char transparency );
//void RenderLine(int x1, int y1, int x2, int y2, wxColour color, int width);
//void GLDrawLine( wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2 );
//void RenderGLText( wxString &msg, wxFont *font, int xp, int yp, double angle);

class transponder_state_history {
public:
  transponder_state_history(){};
  ~transponder_state_history(){};

  int ident;
  int ident_partner;
  int color_index;
  double timeStamp;
  double predicted_lat;
  double predicted_lon;
  double tsh_timer_age;

};

class transponder_state {
public:
  transponder_state(){
    release_status = -2;
    range = 0;
    bearing = 0;
    depth = 0;
    temp = 0;
    timeStamp = 0;
    batt_stat = 0;
    }

  ~transponder_state(){};

  int ident;
  int ident_partner;
  int color_index;
  double timeStamp;
  double predicted_lat;
  double predicted_lon;
  int release_status;
  double range;
  double bearing;
  double depth;
  double temp;
  int batt_stat;
  std::deque<transponder_state_history *> historyQ;

};

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------


class ropeless_pi : public wxTimer, opencpn_plugin_112
{
public:
      ropeless_pi(void *ppimgr);
      ~ropeless_pi(void);

//    The required PlugIn Methods
      int Init(void);
      bool DeInit(void);


      int GetAPIVersionMajor();
      int GetAPIVersionMinor();
      int GetPlugInVersionMajor();
      int GetPlugInVersionMinor();
      wxBitmap *GetPlugInBitmap();
      wxString GetCommonName();
      wxString GetShortDescription();
      wxString GetLongDescription();

//    The optional method overrides
      void SetNMEASentence(wxString &sentence);
      void SetPositionFix(PlugIn_Position_Fix &pfix);
      void SetCursorLatLon(double lat, double lon);
      int GetToolbarToolCount(void);
      void OnToolbarToolCallback(int id);
//      void ShowPreferencesDialog( wxWindow* parent );
      void SetColorScheme(PI_ColorScheme cs);
      void OnContextMenuItemCallback(int id);

      bool RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp);
      bool RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp);
      bool MouseEventHook( wxMouseEvent &event );

//      void OnRolloverPopupTimerEvent( wxTimerEvent& event );
      void PopupMenuHandler( wxCommandEvent& event );

      bool SaveConfig(void);
      void PopulateContextMenu( wxMenu* menu );
      int GetToolbarItemId(){ return m_toolbar_item_id; }
      void SetPluginMessage(wxString &message_id, wxString &message_body);

      void ProcessTimerEvent( wxTimerEvent& event );
      void ProcessSimTimerEvent( wxTimerEvent& event );
//      void RenderFixHat( void );
      void ShowPreferencesDialog( wxWindow* parent );

      //    Secondary thread life toggle
      //    Used to inform launching object (this) to determine if the thread can
      //    be safely called or polled, e.g. wxThread->Destroy();
      void SetSecThreadActive(void){m_bsec_thread_active = true;}
      void SetSecThreadInActive(void){m_bsec_thread_active = false;}
      bool IsSecThreadActive(){ return m_bsec_thread_active; }
      bool                m_bsec_thread_active;
      int                 m_Thread_run_flag;


      void startSim();
      void stopSim();

     int  m_dialogSizeWidth;
     int  m_dialogSizeHeight;
     int  m_dialogPosX;
     int  m_dialogPosY;
     RopelessDialog* m_pRLDialog;

     wxTimer m_simulatorTimer;
     int m_start_sim_id, m_stop_sim_id;
     transponder_state      *m_foundState;
     bool SendReleaseMessage(transponder_state *state, long code);


private:
      bool LoadConfig(void);
      void ApplyConfig(void);

      transponder_state *GetStateByIdent( int identTarget );
      void RenderTransponder(transponder_state *state);
      void RenderTrawls();
      void RenderTrawlConnector( transponder_state *state1, transponder_state *state2 );

      void ProcessRFACapture( void );
      void ProcessRLACapture( void );

      void SaveTransponderStatus();
      void populateTransponderNode(pugi::xml_node &transponderNode,
                                   transponder_state *state);
      void LoadTransponderStatus();
      bool parseTransponderNode(pugi::xml_node &transponderNode,
                            transponder_state *state);

      unsigned char ComputeChecksum( wxString msg );

//      int CalculateFix( void );
//      void setTrackedWPSelect(wxString GUID);

//      void startSerial(const wxString &port);
//      void stopSerial( void );

      wxBitmap         *m_pplugin_icon;
      wxFileConfig     *m_pconfig;
      int              m_toolbar_item_id;

      int               m_show_id;
      int               m_hide_id;

      NMEA0183             m_NMEA0183;                 // Used to parse NMEA Sentences
      // FFU
      int                  m_config_version;
      wxString             m_VDO_accumulator;

      Select               *m_select;

//      ArrayOfBrgLines      m_brg_array;
      double               m_fix_lat;
      double               m_fix_lon;
      ArrayOf2DPoints      m_hat_array;
      int                  m_nfix;
      bool                 m_bshow_fix_hat;

      //        Selection variables
 //     brg_line             *m_sel_brg;
      int                  m_sel_part;
      SelectItem           *m_pFind;
      double               m_sel_pt_lat;
      double               m_sel_pt_lon;
      double               m_segdrag_ref_x;
      double               m_segdrag_ref_y;
      double               m_cursor_lat;
      double               m_cursor_lon;
      int                  m_mouse_x;
      int                  m_mouse_y;

      //        State variables captured from NMEA stream
      int                  mHDx_Watchdog;
      int                  mHDT_Watchdog;
      int                  mGPS_Watchdog;
      int                  mVar_Watchdog;
      double               mVar;
//      double               mHdm;
      double               m_ownship_cog;
      bool                 m_head_active;
      wxTimer              m_head_dog_timer;


   //   int                  mPriPosition;
   //   int                  mPriDateTime;
   //   int                  mPriVar;
   //   int                  mPriHeadingM;
   //   int                  mPriHeadingT;

      double               m_ownship_lat;
      double               m_ownship_lon;
      double               m_hdt;
      wxDateTime           mUTCDateTime;


      //        Rollover Window support
      RolloverWin          *m_pBrgRolloverWin;
      wxTimer               m_RolloverPopupTimer;
      int                   m_rollover_popup_timer_msec;

 //     wxColour              m_FixHatColor;

      PI_EventHandler       *m_event_handler;
      PI_OCP_DataStreamInput_Thread *m_serialThread;
      wxString              m_serialPort;

     unsigned int           m_iconTexture;
     int                    m_texwidth, m_texheight;

     SelectItem             *m_tenderSelect;
     wxString               m_trackedWP;
     wxString               m_trackedWPGUID;

     unsigned int           m_colorIndexNext;

     ODDC                   *m_oDC;

     int                    m_leftclick_tool_id;

     wxWindow               *m_parent_window;
     double                 m_selectRadius;

     wxSocketBase*          m_tsock;
     wxIPV4address          m_tconn_addr;

     DECLARE_EVENT_TABLE();

};

//      An event handler to manage timer ticks, and the like
class PI_EventHandler : public wxEvtHandler
{
public:
    PI_EventHandler( ropeless_pi *parent);
    ~PI_EventHandler();

    void OnTimerEvent( wxTimerEvent& event );
    void PopupMenuHandler(wxCommandEvent& event );
    void OnEvtOCPN_NMEA(PI_OCPN_DataStreamEvent & event);

private:
    ropeless_pi              *m_parent;

    DECLARE_EVENT_TABLE();

};

typedef enum BearingTypeEnum
{
    MAG_BRG = 0,
    TRUE_BRG
}_BearingTypeEnum;

class RopelessDialog : public wxDialog
{
private:

protected:
    wxStdDialogButtonSizer* m_sdbSizer1;
    wxButton* m_sdbSizer1OK;
    wxButton* m_sdbSizer1Cancel;

public:
//     wxRadioBox* m_rbViewType;
//     wxCheckBox* m_cbShowPlotOptions;
//     wxCheckBox* m_cbShowAtCursor;
//     wxCheckBox* m_cbLiveIcon;
//     wxCheckBox* m_cbShowIcon;
//     wxSlider* m_sOpacity;

    wxComboBox *m_comboPort;
    wxArrayString *m_pSerialArray;

    wxComboBox  *m_wpComboPort;

    wxString  m_trackedPointName;
    wxString  m_trackedPointGUID;

    wxComboBox  *m_comboIcon;
    wxTextCtrl *m_pTenderGPSOffsetX;
    wxTextCtrl *m_pTenderGPSOffsetY;
    wxTextCtrl *m_pTenderLength;
    wxTextCtrl *m_pTenderWidth;

    wxTextCtrl *m_simTextCtrl;
    wxButton   *m_ChooseFileButton, *m_StopSimButton, *m_StartSimButton, *m_ManualReleaseButton;

    ropeless_pi *pParentPi;
    OCPNListCtrl *m_pListCtrlTranponders;

    RopelessDialog( wxWindow* parent, ropeless_pi *parent_pi, wxWindowID id = wxID_ANY, const wxString& title = _("Ropeless"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );
    ~RopelessDialog();

    void OnOKClick(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnChooseFileButton(wxCommandEvent &event);
    void OnStopSimButton(wxCommandEvent &event);
    void OnStartSimButton(wxCommandEvent &event);
    void OnManualReleaseButton(wxCommandEvent &event);
    void RefreshTransponderList();
    void OnTargetListColumnClicked(wxListEvent &event);
    void OnTargetRightClick(wxListEvent &event);

    DECLARE_EVENT_TABLE()

};

#endif

