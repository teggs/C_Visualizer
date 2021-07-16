#include "frame.h"
#include <wx/colordlg.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <algorithm>

//  scrollcode, (v1.2)
//  Copyright (C) 2021-, Chris.McDonald@uwa.edu.au

//  ----------------------------------------------------------------------

ScrollCode::ScrollCode(wxWindow *parent, FILE *fp) : wxScrolledCanvas(parent)
{
    wxClientDC dc(this);

//  CHOOSE A FIXED-WIDTH FONT FOR DISPLAYING SOURCE CODE
    InitFont(&dc);

//  REPLACE TABS IN SOURCE CODE BY SPACES
    expandTabs(fp, DEF_TABSTOP);

    Tokenize tokenize;
    tokens      = tokenize.tokenize(lines);

    for(auto &it: tokens) {
        it.x    = (MARGIN_WIDTH+it.col)*fw;
    }
//  SET THE PHYSICAL/VISIBLE SIZE OF OUR WINDOW (INCLUDING SCROLLBARS)
    wxSize  size    = {
        wxSystemSettings::GetMetric(wxSYS_VSCROLL_X) +
                    (int)((MARGIN_WIDTH + std::min(ncols, INIT_NCOLS))*fw),
        wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y) +
                    (int)(std::min(nrows, INIT_NROWS)*fh - fh) + 4
    };
    SetSize(size);

//  SET THE VIRTUAL SIZE OF OUR SOURCE CODE CANVAS
    size    = {
        (int)((MARGIN_WIDTH+ncols)*fw),
        (int)(nrows*fh)
    };
    SetVirtualSize(size);

    colour_margin       = wxColour(COLOUR_MARGIN);
    colour_lineno       = wxColour(COLOUR_LINENO);
    colour_background   = wxColour(COLOUR_BACKGROUND);
    colour_saved        = wxColour(COLOUR_SAVED);

    colour_keyword	= wxColour(COLOUR_KEYWORD);
    colour_identifier	= wxColour(COLOUR_IDENTIFIER);
    colour_number	= wxColour(COLOUR_NUMBER);
    colour_string	= wxColour(COLOUR_STRING);
    colour_preprocessor	= wxColour(COLOUR_PREPROCESSOR);
    colour_comment	= wxColour(COLOUR_COMMENT);
    colour_other	= wxColour(COLOUR_OTHER);

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(colour_background);

    SetScrollbars((int)fw, (int)fh, ncols, nrows);
    SetScrollRate(1, 1);

    rowsaved  = -1;

    Bind(wxEVT_PAINT,	        &ScrollCode::OnPaint,	    this);
    Bind(wxEVT_KEY_DOWN,	&ScrollCode::OnKeyDown,	    this);
    Bind(wxEVT_MOUSEWHEEL,	&ScrollCode::OnMouseWheel,  this);
}

//  CHOOSE A FIXED-WIDTH FONT FOR DISPLAYING SOURCE CODE
void ScrollCode::InitFont(wxDC *dc)
{
#if     defined(__APPLE__)
    char    desc[64];

    sprintf(desc, "monaco %i macroman", INIT_FONT_SIZE);
    font.SetNativeFontInfoUserDesc(desc);
#else
    font    = wxFont(INIT_FONT_SIZE, wxFONTFAMILY_MODERN,
                     wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
#endif
    dc->SetFont(font);

    fontbold    = font.Bold();

    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
    wxColour col    = wxColour("black");
    double  unwanted;

    gc->SetFont(font, col);
    gc->GetTextExtent("g", &fw, &fh, &unwanted, &unwanted);
    fh  = (int)(fh+0.99);
    delete gc;
}

//  REPAINT MARGIN AND VISIBLE LINES OF SOURCE CODE
void ScrollCode::OnPaint(wxPaintEvent& UNUSED(event))
{
    int r0      = GetViewStart().y / fh;
    int r1      = std::min(nrows, r0 + (int)(GetClientSize().y / fh) + 1);

    SetBackgroundColour(colour_background);

    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    dc.SetFont(font);

//  PAINT THE MARGIN, POSSIBLY A THIN LINE AT RIGHT
    dc.SetBrush(colour_margin);
    dc.SetPen(colour_margin);
    dc.DrawRectangle(0, 0, MARGIN_WIDTH*fw, GetClientSize().y);
#if defined(LINE_RHS)
    dc.DrawRectangle((MARGIN_WIDTH+LINE_RHS)*fw, 0, 1, GetClientSize().y);
#endif

//  DRAW LINE NUMBERS IN MARGIN
    dc.SetTextBackground(colour_background);
    dc.SetTextForeground(colour_lineno);
    wxCoord y   = 0;

    for(int r=r0 ; r<r1 ; ++r) {
        if(!lines[r].empty()) {
            char    str[8];

            sprintf(str, MARGIN_I_FORMAT, r+1);
            dc.DrawText(str, 1, y);
        }
	y	+= fh;
    }

//  POSSIBLY DRAW A SAVED ROW
    if(r0 <= rowsaved && rowsaved < r1) {
        dc.SetBrush(colour_saved);
        dc.SetPen(colour_saved);
        dc.DrawRectangle(MARGIN_WIDTH*fw, (rowsaved-r0)*fh, GetClientSize().x, fh);
    }

    DrawSourceTokens(&dc, r0, r1);
}

//  DRAW THE VISIBLE SOURCE CODE TOKENS
void ScrollCode::DrawSourceTokens(wxDC *dc, int r0, int r1)
{
    wxColour    fgnow    = wxNullColour;
    wxColour    fgwanted;

    dc->SetTextBackground(colour_background);
    for(const auto &it: tokens) {
        if(it.row >= r1) {
            break;
        }
        if(it.row >= r0) {
            switch (it.token) {
            case T_KEYWORD :
                    fgwanted	= colour_keyword;
                    break;
            case T_IDENTIFIER :
                    fgwanted	= colour_identifier;
                    break;
            case T_NUMBER :
                    fgwanted	= colour_number;
                    break;
            case T_STRING :
                    fgwanted	= colour_string;
                    break;
            case T_PREPROCESSOR :
                    fgwanted	= colour_preprocessor;
                    break;
            case T_COMMENT :
                    fgwanted	= colour_comment;
                    break;
            default :
                    fgwanted	= colour_other;
                    break;
            }
            if(fgnow != fgwanted) {
                dc->SetTextForeground(fgnow = fgwanted);
            }
            if(it.token == T_KEYWORD) {
                dc->SetFont(fontbold);
            }
            dc->DrawText(it.str, it.x, (it.row-r0)*fh);
            if(it.token == T_KEYWORD) {
                dc->SetFont(font);
            }
        }
    }
}

//  ----------------------------------------------------------------------

void ScrollCode::OnKeyDown(wxKeyEvent &event)
{
    wxChar uc = event.GetUnicodeKey();

    if(uc != WXK_NONE) {                // It's a "normal" char, control-char
        switch (uc) {
        case WXK_ESCAPE :
            wxExit();
            break;
        }
    }
    else {
        int newx = -1, newy = -1;

        switch (event.GetKeyCode()) {
        case WXK_LEFT:      newx    = GetViewStart().x - fw;
                            break;
        case WXK_RIGHT:     newx    = GetViewStart().x + fw;
                            break;
        case WXK_UP:        newy    = GetViewStart().y - fh;
                            break;
        case WXK_DOWN:      newy    = GetViewStart().y + fh;
                            break;
        case WXK_PAGEUP:    newy    = GetViewStart().y - GetClientSize().y;
                            break;
        case WXK_PAGEDOWN:  newy    = GetViewStart().y + GetClientSize().y;
                            break;
        }
        if(newx != -1 || newy != -1) {
            Scroll(newx, newy);
        }
    }
}

//  A MOUSE CLICK, EITHER WITHIN THE MINIVIEWER OR THE LINES OF CODE
void ScrollCode::OnMouseLeftUp()
{ 
    Refresh();
}

//  USING THE MOUSE TO SCROLL UP OR DOWN, ONE LINE AT A TIME
void ScrollCode::OnMouseWheel(wxMouseEvent& event)
{
    int dy  = (event.GetWheelRotation() < 0) ? fh : -fh;

    Scroll(-1, GetViewStart().y + dy);
}

void ScrollCode::OnSetColours(wxCommandEvent& event)
{
    wxColourData    colourData;

//  SET THE DEFAULT/INITIAL COLOUR OF THE ColourDialog
    switch (event.GetId()) {
    case ID_SET_COLOUR_BACKGROUND :     colourData.SetColour(colour_background);
                                        break;
    case ID_SET_COLOUR_MARGIN :         colourData.SetColour(colour_margin);
                                        break;
    case ID_SET_COLOUR_LINENO :         colourData.SetColour(colour_lineno);
                                        break;
    case ID_SET_COLOUR_SAVED :          colourData.SetColour(colour_saved);
                                        break;
    case ID_SET_COLOUR_KEYWORD :        colourData.SetColour(colour_keyword);
                                        break;
    case ID_SET_COLOUR_IDENTIFIER :     colourData.SetColour(colour_identifier);
                                        break;
    case ID_SET_COLOUR_NUMBER :         colourData.SetColour(colour_number);
                                        break;
    case ID_SET_COLOUR_STRING :         colourData.SetColour(colour_string);
                                        break;
    case ID_SET_COLOUR_PREPROCESSOR :   colourData.SetColour(colour_preprocessor);
                                        break;
    case ID_SET_COLOUR_COMMENT :        colourData.SetColour(colour_comment);
                                        break;
    case ID_SET_COLOUR_OTHER :          colourData.SetColour(colour_other);
                                        break;
    }

    wxColourDialog dialog(this, &colourData);

//  POP-UP dialog
    if(dialog.ShowModal() == wxID_OK) {

//  GET THE SELECTED COLOUR FROM THE dialog
        wxColour newColour   = dialog.GetColourData().GetColour();

        switch (event.GetId()) {
        case ID_SET_COLOUR_BACKGROUND : colour_background = newColour;
                                        break;
        case ID_SET_COLOUR_MARGIN :     colour_margin = newColour;
                                        break;
        case ID_SET_COLOUR_LINENO :     colour_lineno = newColour;
                                        break;
        case ID_SET_COLOUR_SAVED :      colour_saved = newColour;
                                        break;
        case ID_SET_COLOUR_KEYWORD :    colour_keyword = newColour;
                                        break;
        case ID_SET_COLOUR_IDENTIFIER : colour_identifier = newColour;
                                        break;
        case ID_SET_COLOUR_NUMBER :     colour_number = newColour;
                                        break;
        case ID_SET_COLOUR_STRING :     colour_string = newColour;
                                        break;
        case ID_SET_COLOUR_PREPROCESSOR : colour_preprocessor = newColour;
                                        break;
        case ID_SET_COLOUR_COMMENT :    colour_comment = newColour;
                                        break;
        case ID_SET_COLOUR_OTHER :      colour_other = newColour;
                                        break;
        }
        Refresh();
    }
}

//  ----------------------------------------------------------------------

//  REPLACE TABS IN SOURCE CODE BY SPACES
void ScrollCode::expandTabs(FILE *fp, int tabstop)
{
    char	line[BUFSIZ];

    lines.clear();
    nrows	= 0;
    ncols	= 0;
    while(fgets(line, sizeof line, fp)) {
	std::string	expanded;
	int	col	= 0;
	char	*s	= line;

	while(*s && *s != '\n' && *s != '\r') {
	    if(*s == '\t') {
		int nextstop    = ((col/tabstop)+1)*tabstop;
		while(col < nextstop) {
		    expanded.append(1, ' ');
		    ++col;
		}
	    }
	    else {
                expanded.append(1, *s);
                ++col;
            }
            ++s;
	}

	lines.push_back(expanded);
	++nrows;
	ncols	= std::max(ncols, col);
    }
}

//  vim:set sw=4 ts=8: 
