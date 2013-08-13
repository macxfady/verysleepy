/*=====================================================================
optionsdlg.cpp
----------------

Copyright (C) Richard Mitton

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

http://www.gnu.org/copyleft/gpl.html.
=====================================================================*/
#include "optionsdlg.h"
#include "wx/filepicker.h"
#include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"

class wxPercentSlider : public wxSlider
{
public:
    wxPercentSlider(wxWindow *parent,
             wxWindowID id,
             int value,
             int minValue,
             int maxValue,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxSL_HORIZONTAL,
             const wxValidator& validator = wxDefaultValidator,
             const wxString& name = wxSliderNameStr)
    {
        Init();

        Create(parent, id, value, minValue, maxValue, pos, size, style, validator, name);
    }

protected:

	// RJM- had to modify the wxWidgets source slightly to get this to work :-/
	// Just go into include/wx/msw/slider.h, and change the definition of this
	// from 'static' to 'virtual', and add a const modifier. Then you have to do
	// a full 'make clean' and rebuild for it to take.
    virtual wxString Format(int n) const { return wxString::Format(wxT("%d%%"), n); }
};

enum OptionsId
{
	Options_UseSymServer = 1,
	Options_Throttle,
	Options_SymPath,
	Options_SymPath_Add,
	Options_SymPath_Remove,
	Options_SymPath_MoveUp,
	Options_SymPath_MoveDown,
};

BEGIN_EVENT_TABLE(OptionsDlg, wxDialog)
EVT_BUTTON(wxID_OK, OptionsDlg::OnOk)
EVT_CHECKBOX(Options_UseSymServer, OptionsDlg::OnUseSymServer)
EVT_LISTBOX(Options_SymPath, OptionsDlg::OnSymPath)
EVT_BUTTON(Options_SymPath_Add, OptionsDlg::OnSymPathAdd)
EVT_BUTTON(Options_SymPath_Remove, OptionsDlg::OnSymPathRemove)
EVT_BUTTON(Options_SymPath_MoveUp, OptionsDlg::OnSymPathMoveUp)
EVT_BUTTON(Options_SymPath_MoveDown, OptionsDlg::OnSymPathMoveDown)
END_EVENT_TABLE()

OptionsDlg::OptionsDlg()
:	wxDialog(NULL, -1, wxString(_T("Options")), 
			 wxDefaultPosition, wxDefaultSize,
			 wxDEFAULT_DIALOG_STYLE)
{
	wxBoxSizer *rootsizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer *symsizer = new wxStaticBoxSizer(wxVERTICAL, this, "Symbols");
	wxStaticBoxSizer *symdirsizer = new wxStaticBoxSizer(wxVERTICAL, this, "Symbol search path");
	wxStaticBoxSizer *symsrvsizer = new wxStaticBoxSizer(wxVERTICAL, this, "Symbol server");

	symPaths = new wxListBox(this, Options_SymPath, wxDefaultPosition, wxSize(-1, 75), 0, NULL, wxLB_SINGLE | wxLB_NEEDED_SB);

	wxBoxSizer *sympathButtons = new wxBoxSizer(wxHORIZONTAL);
	wxButton *b;
	symPathAdd      = b = new wxButton(this, Options_SymPath_Add     ); b->SetBitmap(LoadPngResource(L"button_add"   )); sympathButtons->Add(b, 1); b->SetToolTip("Browse for a directory to add");
	symPathRemove   = b = new wxButton(this, Options_SymPath_Remove  ); b->SetBitmap(LoadPngResource(L"button_remove")); sympathButtons->Add(b, 1); b->SetToolTip("Remove selected directory");
	symPathMoveUp   = b = new wxButton(this, Options_SymPath_MoveUp  ); b->SetBitmap(LoadPngResource(L"button_up"    )); sympathButtons->Add(b, 1); b->SetToolTip("Move selected directory up");
	symPathMoveDown = b = new wxButton(this, Options_SymPath_MoveDown); b->SetBitmap(LoadPngResource(L"button_down"  )); sympathButtons->Add(b, 1); b->SetToolTip("Move selected directory down");
	UpdateSymPathButtons();

	useSymServer = new wxCheckBox(this, Options_UseSymServer, "Use symbol server");
	symCacheDir = new wxDirPickerCtrl(this, -1, prefs.symCacheDir, "Select a directory to store local symbols in:",
		wxDefaultPosition, wxDefaultSize, wxDIRP_USE_TEXTCTRL);
	symServer = new wxTextCtrl(this, -1, prefs.symServer);

	saveMinidump = new wxCheckBox(this, -1, "Save minidump");
	saveMinidump->SetToolTip(
		"Include a minidump in saved profiling results.\n"
		"This enables the \"Load symbols from minidump\" option,\n"
		"which allows profiling an application on a user machine without symbols,\n"
		"then examining the profile results on a developer machine with symbols.");

	symPaths->Append(wxSplit(prefs.symSearchPath, ';', 0));
	useSymServer->SetValue(prefs.useSymServer);
	symCacheDir->Enable(prefs.useSymServer);
	symServer->Enable(prefs.useSymServer);
	saveMinidump->SetValue(prefs.saveMinidump);

	symdirsizer->Add(symPaths, 0, wxALL|wxEXPAND, 5);
	symdirsizer->Add(sympathButtons, 0, wxALL|wxEXPAND, 5);

	symsrvsizer->Add(useSymServer, 0, wxALL, 5);
	symsrvsizer->Add(new wxStaticText(this, -1, "Local cache directory:"), 0, wxLEFT|wxTOP, 5);
	symsrvsizer->Add(symCacheDir, 0, wxALL|wxEXPAND, 5);
	symsrvsizer->Add(new wxStaticText(this, -1, "Symbol server location:"), 0, wxLEFT|wxTOP, 5);
	symsrvsizer->Add(symServer, 0, wxALL|wxEXPAND, 5);

	symsizer->Add(symdirsizer, 0, wxALL|wxEXPAND, 5);
	symsizer->Add(symsrvsizer, 0, wxALL|wxEXPAND, 5);
	symsizer->Add(saveMinidump, 0, wxALL, 5);

	wxStaticBoxSizer *throttlesizer = new wxStaticBoxSizer(wxVERTICAL, this, "Sample rate control");
	throttle = new wxPercentSlider(this, Options_Throttle, prefs.throttle, 1, 100, wxDefaultPosition, wxDefaultSize,
		wxSL_HORIZONTAL|wxSL_TICKS|wxSL_TOP|wxSL_LABELS);
	throttle->SetTickFreq(10);
	throttlesizer->Add(new wxStaticText(this, -1, 
		"Adjusts the sample rate speed. Useful for doing longer captures\n"
		"where you wish to reduce the profiler overhead.\n"
		"Higher values increase accuracy; lower values result in better\n"
		"performance."), 0, wxALL, 5);
	throttlesizer->Add(throttle, 0, wxEXPAND|wxLEFT|wxTOP, 5);

	topsizer->Add(symsizer, 0, wxEXPAND|wxALL, 0);
	topsizer->AddSpacer(5);
	topsizer->Add(throttlesizer, 0, wxEXPAND|wxALL, 0);
	rootsizer->Add(topsizer, 1, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 10);
	rootsizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxALL, 10);
	SetSizer(rootsizer);
	rootsizer->SetSizeHints(this);
	SetAutoLayout(TRUE);

	SetSize(wxSize(400, -1));
	Centre();
}

OptionsDlg::~OptionsDlg()
{
}

void OptionsDlg::OnOk(wxCommandEvent& event)
{
	prefs.symSearchPath = wxJoin(symPaths->GetStrings(), ';', 0);
	prefs.useSymServer = useSymServer->GetValue();
	prefs.symCacheDir = symCacheDir->GetPath();
	prefs.symServer = symServer->GetValue();
	prefs.saveMinidump = saveMinidump->GetValue();
	prefs.throttle = throttle->GetValue();
	EndModal(wxID_OK);
}

void OptionsDlg::OnUseSymServer(wxCommandEvent& event)
{
	bool enabled = useSymServer->GetValue();
	symCacheDir->Enable(enabled);
	symServer->Enable(enabled);
}

void OptionsDlg::UpdateSymPathButtons()
{
	int sel = symPaths->GetSelection();
	symPathRemove  ->Enable(sel >= 0);
	symPathMoveUp  ->Enable(sel > 0);
	symPathMoveDown->Enable(sel >= 0 && sel < (int)symPaths->GetCount()-1);
}

void OptionsDlg::OnSymPath( wxCommandEvent & event )
{
	UpdateSymPathButtons();
}

void OptionsDlg::OnSymPathAdd( wxCommandEvent & event )
{
	wxDirDialog dlg(this, "Select a symbol search path to add");
	if (dlg.ShowModal()==wxID_OK)
	{
		int sel = symPaths->Append(dlg.GetPath());
		symPaths->Select(sel);
		UpdateSymPathButtons();
	}
}

void OptionsDlg::OnSymPathRemove( wxCommandEvent & event )
{
	int sel = symPaths->GetSelection();
	symPaths->Delete(sel);
	if (sel < (int)symPaths->GetCount())
		symPaths->Select(sel);
	UpdateSymPathButtons();
}

void OptionsDlg::OnSymPathMoveUp( wxCommandEvent & event )
{
	int sel = symPaths->GetSelection();
	wxString s = symPaths->GetString(sel);
	symPaths->Delete(sel);
	symPaths->Insert(s, sel-1);
	symPaths->Select(sel-1);
	UpdateSymPathButtons();
}

void OptionsDlg::OnSymPathMoveDown( wxCommandEvent & event )
{
	int sel = symPaths->GetSelection();
	wxString s = symPaths->GetString(sel);
	symPaths->Delete(sel);
	symPaths->Insert(s, sel+1);
	symPaths->Select(sel+1);
	UpdateSymPathButtons();
}
