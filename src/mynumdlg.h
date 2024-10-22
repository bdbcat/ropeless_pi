#ifndef __MYNUMDLGH_G__
#define __MYNUMDLGH_G__

#include "wx/defs.h"

#include "wx/dialog.h"

#if wxUSE_SPINCTRL
    class WXDLLIMPEXP_FWD_CORE wxSpinCtrl;
#else
    class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
#endif // wxUSE_SPINCTRL

// ----------------------------------------------------------------------------
// myNumberEntryDialog: a dialog with spin control, [ok] and [cancel] buttons
// ----------------------------------------------------------------------------

class myNumberEntryDialog : public wxDialog
{
public:
    myNumberEntryDialog()
    {
        m_value = m_min = m_max = 0;
    }

    myNumberEntryDialog(wxWindow *parent,
                        const wxString& message,
                        const wxString& prompt,
                        const wxString& caption,
                        long value, long min, long max,
                        const wxPoint& pos = wxDefaultPosition)
    {
        Create(parent, message, prompt, caption, value, min, max, pos);
    }

    bool Create(wxWindow *parent,
                const wxString& message,
                const wxString& prompt,
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
    DECLARE_DYNAMIC_CLASS(myNumberEntryDialog)
    wxDECLARE_NO_COPY_CLASS(myNumberEntryDialog);
};

// ----------------------------------------------------------------------------
// function to get a number from user
// ----------------------------------------------------------------------------

long
    myGetNumberFromUser(const wxString& message,
                        const wxString& prompt,
                        const wxString& caption,
                        long value = 0,
                        long min = 0,
                        long max = 100,
                        wxWindow *parent = NULL,
                        const wxPoint& pos = wxDefaultPosition);


#endif // __MYNUMDLGH_G__
