#ifndef __MYOKDLGH_G__
#define __MYOKDLGH_G__

#include "wx/defs.h"

#include "wx/dialog.h"

#if wxUSE_SPINCTRL
    class WXDLLIMPEXP_FWD_CORE wxSpinCtrl;
#else
    class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
#endif // wxUSE_SPINCTRL

// ----------------------------------------------------------------------------
// myOkDialog: a dialog with [ok] and [cancel] buttons
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE myOkDialog : public wxDialog
{
public:
    myOkDialog()
    {
        m_value = m_min = m_max = 0;
    }

    myOkDialog(wxWindow *parent,
                        const wxString& message,
                        const wxString& caption,
                        long value, long min, long max,
                        const wxPoint& pos = wxDefaultPosition)
    {
        Create(parent, message, caption, value, min, max, pos);
    }

    bool Create(wxWindow *parent,
                const wxString& message,
                const wxString& caption,
                long value, long min, long max,
                const wxPoint& pos = wxDefaultPosition);

    long GetValue() const { return m_value; }

    // implementation only
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

protected:

#if wxUSE_SPINCTRL
    wxSpinCtrl *m_spinctrl;
#else
    wxTextCtrl *m_spinctrl;
#endif // wxUSE_SPINCTRL

    long m_value, m_min, m_max;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(myOkDialog)
    wxDECLARE_NO_COPY_CLASS(myOkDialog);
};

#endif // __MYOKDLGH_G__
