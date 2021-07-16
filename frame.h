#pragma once
#include "wxstuff.h"
#include "tokens.h"
#include <list>

#define APPNAME			"C Visualizer"
#define APPVERSION		"v1.0"
#define AUTHOR_EMAIL	    	"Chris.McDonald@uwa.edu.au"

#define	GDB_PATH		"/usr/bin/gdb"
#define	GDB_NAME		"gdb"
#define GDB_PROMPT              "(gdb)"


#define	OUTPUT_COLS		50
#define	OUTPUT_ROWS		40
#define	OUTPUT_BACKGROUND	"#ccffee"
#define DISPLAY_BACKGROUND  	"#faf9de"
#define	OUTPUT_FOREGROUND	"#7fffd4"
#define	OUTPUT_FREQ		50		// msec
#define	SIZER_BORDER        	3

#define INIT_NROWS              40
#define INIT_NCOLS              90

#if     defined(__linux__)
#define INIT_FONT_SIZE          12
#else
#define INIT_FONT_SIZE          14
#endif

#define	DEF_TABSTOP		8

#define MARGIN_WIDTH            5
#define MARGIN_I_FORMAT         "%4i"

#define LINE_RHS                80

#define MV_WIDTH                80
#define MV_HEIGHT               120
#define MV_ALPHA                120

#define COLOUR_MARGIN           "#aaaaaa"
#define COLOUR_LINENO           "#7fffd4"
#define	COLOUR_BACKGROUND	"#fffce6"
#define	COLOUR_SAVED		"#80ff00"
#define COLOUR_RETURN		"#ffa07a"
#define COLOUR_LOCK		"#d3d3d3"

#define COLOUR_PREPROCESSOR     "purple"
#define COLOUR_KEYWORD          "black"
#define COLOUR_IDENTIFIER       "orange"
#define COLOUR_NUMBER           "pink"
#define COLOUR_STRING           "green"
#define COLOUR_COMMENT          "blue"
#define COLOUR_OTHER            "black"

//  ----------------------------------------------------------------------
enum {
    ID_SET_COLOURS = wxID_HIGHEST + 1,
    ID_SET_COLOUR_BACKGROUND,
    ID_SET_COLOUR_MARGIN,
    ID_SET_COLOUR_LINENO,
    ID_SET_COLOUR_SAVED,

    ID_SET_COLOUR_PREPROCESSOR,
    ID_SET_COLOUR_KEYWORD,
    ID_SET_COLOUR_IDENTIFIER,
    ID_SET_COLOUR_NUMBER,
    ID_SET_COLOUR_STRING,
    ID_SET_COLOUR_COMMENT,
    ID_SET_COLOUR_OTHER
};

//  THE GUI CLASSES ARE DECLARED HERE, AND DEFINED/IMPLEMENTED ELSEWHERE
class ScrollCode : public wxScrolledCanvas
{
public:
    ScrollCode(wxWindow* parent, FILE* fp);
    void        OnSetColours(wxCommandEvent& WXUNUSED(event));
    int 	rowsaved;
    void        OnMouseLeftUp();
private:
    void	InitFont(wxDC* dc);
    void	OnPaint(wxPaintEvent& event);
    void        OnKeyDown(wxKeyEvent& event);
   // void        OnMouseLeftUp(wxMouseEvent& event);
    void        OnMouseWheel(wxMouseEvent& event);
    void	expandTabs(FILE* fp, int tabstop);
    void        DrawSourceTokens(wxDC* dc, int r0, int r1);

    wxFont                      font, fontbold;
    double			fw, fh;

    std::vector<std::string>	lines;
    std::vector<TOKEN>          tokens;
    int			        nrows, ncols;
//    int                         rowsaved;

    //  HMMM, MAYBE WE SHOULD HAVE AN ARRAY OF wxColours?
    wxColour                    colour_background;
    wxColour                    colour_margin;
    wxColour                    colour_lineno;
    wxColour                    colour_saved;

    wxColour                    colour_preprocessor;
    wxColour                    colour_keyword;
    wxColour                    colour_identifier;
    wxColour                    colour_number;
    wxColour                    colour_string;
    wxColour                    colour_comment;
    wxColour                    colour_other;
};

class InPanel : public wxPanel
{
public:
    InPanel(wxWindow* parent, int);
    wxTextCtrl* ctrl;
private:
    void SendGDB(wxString cmd);
    void Clicked_btn(wxCommandEvent&);
    void Clicked_textbox(wxCommandEvent&);

    int			fd_togdb;
};
//  THE GUI CLASSES ARE DEFINED HERE, AND IMPLEMENTED IN gui.cpp
class OutPanel : public wxPanel
{
public:
    OutPanel(wxWindow* parent, int);
    void AddLine(wxString line);
    void ExpectOutput(void);

private:
    void OnPaint(wxPaintEvent&);
    void OnSizeEvent(wxSizeEvent&);
    void PollGDBOutput(wxTimerEvent&);
    void GetInfo(wxString);
    int GetPos(wxString[], int, int);

    wxTimer* timer;
    std::list<wxString>	lines;
    std::list<wxString> stack_to_be_drawn;
    wxString last_level;

    struct StackInfo
    {
	wxString stackname;
	wxString current_return;
	int len_n;
	int len_v;
 	std::list<wxString> argnames;
 	std::list<wxString> argvalues;
    };
    std::list<StackInfo> runtime_stack;

    int			y;
    int                 stack_level; 
    size_t		nrows;
    int			fw, fh;
    
    int			fd_fromgdb;
    char		line[1024], * lp;
    int			gdb_prompt_len;
};

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString&, int, int, FILE* fp);

private:
    void OnAbout(wxCommandEvent&);
    static const char* const   setColourLabels[];
};

//  ----------------------------------------------------------------------

//  GLOBAL VARIABLES DECLARED HERE, DEFINED
extern	OutPanel*  outpanel;
extern InPanel* inpanel;
extern ScrollCode* sc; 
