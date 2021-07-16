#include "frame.h"
#define	INPUT_BACKGROUND	"#9999ff"
#define	INPUT_HINT		"enter a gdb command"

 
std::map<wxString, wxString> label_map = { {wxT("start"),wxT("start")}, {wxT("next"),wxT("step")},
 		{wxT("end"),wxT("finish")}, {wxT("quit"),wxT("quit")} }; 
//Create InPanel here
InPanel::InPanel(wxWindow* parent, int fd_togdb) :
    wxPanel(parent, wxID_ANY)
{
    this->fd_togdb = fd_togdb;

    SetBackgroundColour(INPUT_BACKGROUND);

#define	N_BTNS		4
    wxButton* btn[N_BTNS];
    const char* labels[N_BTNS] = {"start", "next" ,"end", "quit" };

    for (int b = 0; b < N_BTNS; ++b) {
        btn[b] = new wxButton(this, wxID_ANY, labels[b]);
        btn[b]->Bind(wxEVT_BUTTON, &InPanel::Clicked_btn, this);
    }

    wxStaticText* label1 = new wxStaticText(this, wxID_ANY, APPNAME);
    wxStaticText* label2 = new wxStaticText(this, wxID_ANY, APPVERSION);
//    ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
//        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
//    ctrl->SetHint(INPUT_HINT);
//    ctrl->Bind(wxEVT_TEXT_ENTER, &InPanel::Clicked_textbox, this);

    //  MANAGE THE LAYOUT OF THIS PANEL
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(btn[0], 0, wxEXPAND | wxALL, SIZER_BORDER);
   // sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxALL, SIZER_BORDER);
   // sizer->Add(ctrl, 1, wxEXPAND | wxALL, SIZER_BORDER);
    sizer->Add(btn[1], 0, wxEXPAND | wxALL, SIZER_BORDER);
    sizer->Add(btn[2], 0, wxEXPAND | wxALL, SIZER_BORDER);
    sizer->Add(btn[3], 0, wxEXPAND | wxALL, SIZER_BORDER);
   // sizer->Add(btn[4], 0, wxEXPAND | wxALL, SIZER_BORDER);
    sizer->Add(label2, 0, wxALIGN_CENTER_VERTICAL | wxALL, SIZER_BORDER); 
    sizer->Add(label1, 0, wxALIGN_CENTER_VERTICAL | wxALL, SIZER_BORDER); 
    SetSizerAndFit(sizer);
}

//  SENDS A COMMAND TO gdb AND DISPLAYS IT ON THE OutPanel
void InPanel::SendGDB(wxString cmd)
{
    const char* str = (const char*)cmd.c_str();
    write(fd_togdb, str, strlen(str));
    write(fd_togdb, "\n", 1);
    if (cmd.compare("quit") == 0) {
        wxExit();
    }
    else {
        //outpanel->AddLine(cmd);
	//outpanel->AddLine(wxT("just a test"));
        sc->OnMouseLeftUp();
        outpanel->ExpectOutput();
    }
}

//  SENDS THE LABEL ON EACH BUTTON TO gdb
void InPanel::Clicked_btn(wxCommandEvent& event)
{
    wxButton* btn = (wxButton*)event.GetEventObject();

    SendGDB(label_map[btn->GetLabel()]);
    SendGDB(wxT("p $rax"));
    SendGDB(wxT("i locals"));
    SendGDB(wxT("i args"));
    SendGDB(wxT("bt"));
}

//  SENDS THE ENTERED TEXT AS A COMMAND TO gdb
void InPanel::Clicked_textbox(wxCommandEvent& UNUSED(event))
{
    if (!ctrl->IsEmpty()) {
        SendGDB(ctrl->GetValue());
        ctrl->Clear();
    }
}
