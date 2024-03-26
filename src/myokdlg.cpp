/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/numdlgg.cpp
// Purpose:     wxGetNumberFromUser implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.07.99
// Copyright:   (c) Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <stdio.h>

    #include "wx/utils.h"
    #include "wx/dialog.h"
    #include "wx/button.h"
    #include "wx/stattext.h"
    #include "wx/textctrl.h"
    #include "wx/intl.h"
    #include "wx/sizer.h"
#endif

#if wxUSE_STATLINE
  #include "wx/statline.h"
#endif

#if wxUSE_SPINCTRL
#include "wx/spinctrl.h"
#endif

// this is where wxGetNumberFromUser() is declared
#include "myokdlg.h"

#if !wxUSE_SPINCTRL
    // wxTextCtrl will do instead of wxSpinCtrl if we don't have it
    #define wxSpinCtrl wxTextCtrl
#endif

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxNumberEntryDialog
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(myOkDialog, wxDialog)
    EVT_BUTTON(wxID_OK, myOkDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, myOkDialog::OnCancel)
END_EVENT_TABLE()

IMPLEMENT_CLASS(myOkDialog, wxDialog)

bool myOkDialog::Create(wxWindow *parent,
                                         const wxString& message,
                                         const wxString& caption,
                                         long value,
                                         long min,
                                         long max,
                                         const wxPoint& pos)
{
    if ( !wxDialog::Create(GetParentForModalDialog(parent, 0),
                           wxID_ANY, caption,
                           pos, wxDefaultSize) )
    {
        return false;
    }

    m_value = value;
    m_max = max;
    m_min = min;

    wxBeginBusyCursor();

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

#if wxUSE_STATTEXT
    // 1) text message
    topsizer->Add( CreateTextSizer( message ), 0, wxALL, 10 );
#endif

    // 3) buttons if any
    wxSizer *buttonSizer = CreateSeparatedButtonSizer(wxOK | wxCANCEL);
    if ( buttonSizer )
    {
        topsizer->Add(buttonSizer, wxSizerFlags().Expand().DoubleBorder());
    }

    SetSizer( topsizer );
    SetAutoLayout( true );

    topsizer->SetSizeHints( this );
    topsizer->Fit( this );

    Centre( wxBOTH );

    wxEndBusyCursor();

    return true;
}

void myOkDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
// #if !wxUSE_SPINCTRL
//     wxString tmp = m_spinctrl->GetValue();
//     if ( wxSscanf(tmp, wxT("%ld"), &m_value) != 1 )
//         EndModal(wxID_CANCEL);
//     else
// #else
//     m_value = m_spinctrl->GetValue();
// #endif
//     if ( m_value < m_min || m_value > m_max )
//     {
//         // not a number or out of range
//         m_value = -1;
//         EndModal(wxID_CANCEL);
//     }

    EndModal(wxID_OK);
}

void myOkDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_CANCEL);
}

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// wxGetTextFromUser is in utilscmn.cpp

long myGetNumberFromUser(const wxString& msg,
                         const wxString& title,
                         long value,
                         long min,
                         long max,
                         wxWindow *parent,
                         const wxPoint& pos)
{
    myOkDialog dialog(parent, msg, title,
                               value, min, max, pos);
    if (dialog.ShowModal() == wxID_OK)
        return dialog.GetValue();

    return -1;
}
