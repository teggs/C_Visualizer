#include "frame.h"

int MG = 150;
int mg1 = 10;
int mg2 = 5;
int arrow_s = 60;
wxColour clr = wxColour("black");
//wxFont ft = wxFont(12, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false);
//wxFont ftb = ft.Bold();
//  THE OutPanel DISPLAYS THE OUTPUT RECEIVED FROM gdb
OutPanel::OutPanel(wxWindow* parent, int fd_fromgdb) :
    wxPanel(parent, wxID_ANY)
{
    this->stack_level = 0;
    this->last_level = wxT("0 main");
    this->fd_fromgdb = fd_fromgdb;
    std::list<wxString> empty;
    StackInfo base = {wxT("0 main"), wxT("None"), 0, 0, empty, empty};
    runtime_stack.push_back(base);

    wxClientDC dc(this);
    SetFont(wxFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false)); 

    nrows = OUTPUT_ROWS;
    dc.GetTextExtent("M", &fw, &fh);		// find font width and height
    SetMinSize(wxSize(OUTPUT_COLS * fw, nrows * fh));

    SetBackgroundColour(OUTPUT_BACKGROUND);
    lines.clear();

    lp = line;
    gdb_prompt_len = strlen(GDB_PROMPT);

    Bind(wxEVT_PAINT, &OutPanel::OnPaint, this);
    Bind(wxEVT_SIZE, &OutPanel::OnSizeEvent, this);

    timer = new wxTimer(this);
    Bind(wxEVT_TIMER, &OutPanel::PollGDBOutput, this);
    timer->Start(OUTPUT_FREQ, wxTIMER_CONTINUOUS);
}

//  START POLLING FOR OUTPUT FROM gdb
void OutPanel::ExpectOutput(void)
{
    timer->Start(OUTPUT_FREQ, wxTIMER_CONTINUOUS);
}

//  ADD A LINE TO THE LIST OF OUTPUT LINES
void OutPanel::AddLine(wxString line)
{
    if (lines.size() >= nrows)
        lines.pop_front();
    lines.push_back(line);
    Refresh();					// force wxEVT_PAINT
}

//  REPAINT ALL OUTPUT LINES
void OutPanel::OnPaint(wxPaintEvent& UNUSED(event))
{
    wxPaintDC dc(this);
    wxFont ft = wxFont(12, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false); 
    wxFont ftb = ft.Bold();
    wxFont ft1 = wxFont(16, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false);
    wxFont ft1b = ft1.Bold(); 
    wxFont ft2 = wxFont(11, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false);
    //draw the background
    wxString label1 = wxT("Addr");
    wxString label2 = wxT("Stack");
    wxString label3 = wxT("Bot");
    wxString label4 = wxT("Top");
    dc.SetTextForeground(wxColour("red"));
    dc.SetFont(ft1b);
    dc.DrawText(label1, mg1*3, fh);
    dc.DrawText(label2, MG, fh);
    dc.SetFont(ft2);
    dc.DrawText(label3, mg1*5, fh*3);
	    
    dc.SetFont(ft);
    dc.SetTextForeground(wxColour("black"));
    y = 2*fh;
    //reset the level and to draw info
    this->stack_level = 0;
    this->stack_to_be_drawn.clear();
    // list recent gdb lines
    for (std::list<wxString>::reverse_iterator it = lines.rbegin(); it != lines.rend(); ++it)
    {
	wxString temp = *it;
        const char* tempstr = (const char*)temp.c_str();
        if (tempstr[0]>=48 && tempstr[0]<=57)
	{
	    sc->rowsaved = atoi(tempstr) - 1;
	    break;
	} 
	GetInfo(temp);//update the to draw list
    }

    //NOW draw the stack during the runtime 
    wxString name_to_draw = wxT("0 main");	
    if (stack_to_be_drawn.size()>0)
    	name_to_draw = stack_to_be_drawn.back();
    int flag = name_to_draw.compare(this->last_level); // Is the current last frame the same as the previous last frame?
    wxStringTokenizer findcount(last_level, " ");
    int frame_count = wxAtoi(findcount.GetNextToken()); // the frame count of the last stack
    this ->last_level = name_to_draw; //update the last frame

    int total_frames = static_cast<int>(stack_to_be_drawn.size());
    bool mark = ((flag != 0) && total_frames <= frame_count+1); // the last frame is different and the frame is decreasing
    
    int ct = 0;
    for (std::list<wxString>::iterator it = stack_to_be_drawn.begin(); it != stack_to_be_drawn.end(); ++it)
    {   ct++;
	if (ct == total_frames)//the last frame
	{
	    clr = wxColour(OUTPUT_FOREGROUND);
	}else{
	    clr = wxColour(COLOUR_LOCK);
	}
	dc.SetBrush(clr);
	y += fh;
	name_to_draw = *it;
	StackInfo current_s;
        for (std::list<StackInfo>::reverse_iterator rit = runtime_stack.rbegin(); rit != runtime_stack.rend(); ++rit)
	{
	    current_s = *rit;
	    if (current_s.stackname == name_to_draw)
	    {
		break;
	    }
	}
	//Now draw this frame
	int len_name = (current_s.len_n + 2)*10; //2 is the space for ": " 
	int width = (current_s.len_v)*10 + len_name + 2*mg1;
	std::list<wxString>list1 = current_s.argnames;
	std::list<wxString>list2 = current_s.argvalues;
	
	wxStringTokenizer tokenizer(current_s.stackname, " ");
//	tokenizer.GetNextToken(); 
	int frame_draw = wxAtoi(tokenizer.GetNextToken());
	wxString name = tokenizer.GetNextToken();
	int width1 = name.Len()*10 + 2*mg2;
 	dc.SetFont(ftb);
        dc.DrawRectangle(MG, y, width1, fh*2);
	dc.DrawText(name, MG+mg2, y+fh-mg1);
	
	dc.SetFont(ft);
	if (mark && frame_draw == frame_count-1) //this suggests one frame was destructed i.e. something was returned
	{   
	    int r_s = width1 + MG + arrow_s;
	    wxString c_r = current_s.current_return;
	    dc.SetBrush(wxColour(COLOUR_RETURN));
	    dc.SetPen(wxColour(COLOUR_RETURN)); 
	    dc.DrawRoundedRectangle(r_s, y+mg2, (c_r.Len()*10+2*mg1), (fh-mg2)*2,8);
	    dc.DrawText(c_r, r_s+mg1, y+fh-mg1);
 
            dc.DrawRectangle(width1+MG+mg1*2, y+fh, arrow_s/2, fh/3);
            double clx0 = width1+MG+mg1*2;
	    double cly0 = y+fh+fh/6;
            const wxPoint points0[3] = { wxPoint(clx0, cly0-fh/3), wxPoint(clx0, cly0+fh/3), wxPoint(clx0-0.577*fh, cly0) };
            dc.DrawPolygon(3, points0);
	}
	dc.SetBrush(clr);
	dc.SetPen(wxColor("black"));
	y += fh*2;  
	for(std::list<wxString>::iterator it1=list1.begin(), it2=list2.begin(); (it1!=list1.end())&&(it2!=list2.end()); ++it1, ++it2)
	{
	    wxString c_n = *it1;
            if (c_n[0] != '$')
	    {
	        dc.DrawRectangle(MG, y, width, fh*2);
	        dc.DrawText(wxString::Format(("%s%s"),*it1,wxT(": ")), MG+mg1, y+fh-mg2);
	        dc.DrawText(*it2, MG+mg1+len_name, y+fh-mg2);
	        y+=fh*2;
	    }
	}
    }
    //draw a BIG arrow!
    dc.SetBrush(wxColour("red"));
    dc.SetPen(wxColour("red"));
    int len_r = std::max(40, static_cast<int>((y-fh*3)*0.8));
    dc.DrawRectangle(mg1*8, fh*3+mg2, mg1, len_r);

    double clx = mg1*8.5;
    double cly = fh*3 + mg2 + len_r;
    const wxPoint points1[3] = { wxPoint(clx-mg1, cly), wxPoint(clx+mg1, cly), wxPoint(clx, cly+1.73*mg1) };
    dc.DrawPolygon(3, points1); 
    dc.SetBrush(clr);
    dc.SetPen(clr);

    dc.SetTextForeground(wxColour("red"));
    dc.SetFont(ft2);
    dc.DrawText(label4, mg1*5, fh*2+len_r);
    dc.SetFont(ft);
    dc.SetTextForeground(wxColour("black"));
}

void OutPanel::GetInfo(wxString temp)
{
    int array_i = 0;
    int size = 0;
    int pos = 0;
    wxString stackname = wxT("error");

    std::list<wxString> wordlist;
    wxStringTokenizer tokenizer(temp, " ");
    while ( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();
        if(token.compare(wxT("(gdb)")) != 0)
	{
 	    wordlist.push_back(token);
        }
    }
    size = wordlist.size();
    if (size < 3)//useful list has at least 3 elements like: a = 1
    {
	return;
    }

    wxString wordarray[size];//convert the list to an array.
    for(std::list<wxString>::iterator it = wordlist.begin(); it != wordlist.end(); ++it)
    {
	wordarray[array_i] = *it;
	array_i++;
    }

    //NOW process the array
    std::string wordhead = std::string(wordarray[0].mb_str());
    if (wordhead[0] == 35) //line starts with "#"
    {  
	pos = GetPos(wordarray, size, 40); // find the position of '('
	if(pos == -1)
	{
	    return;
	}
	else
	{
	    stackname = wordarray[pos-1];
	    this->stack_to_be_drawn.push_back(wxString::Format(("%s %s"),wxString::Format(wxT("%i"),this->stack_level),stackname));
	    this->stack_level++;
	}

	if (wordhead[1] == 48) //line starts with "#0",indicates that it is the last info about stack
	{
	    wxString current_stack = stack_to_be_drawn.back();
	    std::list<wxString> empty;
	    StackInfo frame = {current_stack,wxT("None"), 0, 0, empty, empty};
	    this->runtime_stack.push_back(frame);
	    return; 
	}
    }
    else // line starts without "#" like 'a = 1'
    {
        StackInfo current_frame = runtime_stack.back();
	wxString name;
	wxString value;

        pos = GetPos(wordarray, size, 61); //find the position of '='
	if(pos == -1)
	{
	    return;
	}
	if (wordhead[0] ==36)//line starts with '$', indicates that it is the value from rax.
	{
	    value = wordarray[pos+1];
	    current_frame.current_return = wxString::Format(("%s %s"),wxT("Returned:"),value);
	}
	else
	{
	    name = wordarray[pos-1];
	    value = wordarray[pos+1];
	    std::string value0 = std::string(value.mb_str());
	    if (value0.length()>2 && value0[0] == '0' && value0[1] == 'x')
	    {
		value = wxT("Pointer");
	    }
	    int len1 = name.Len();
	    int len2 = value.Len();
	    if (len1 > current_frame.len_n)
		current_frame.len_n = len1;
            if (len2 > current_frame.len_v)
	        current_frame.len_v = len2;
	    current_frame.argnames.push_back(name);
	    current_frame.argvalues.push_back(value);
	}
	runtime_stack.pop_back();
	runtime_stack.push_back(current_frame);
    }
    return;
}

int OutPanel::GetPos(wxString wordarray[], int size, int symbol)
{	
    for ( int i=0; i<size-1; i++)
    {
	if( wordarray[i].c_str()[0] == symbol)
	{
	    return i;
	}
    }
    return -1;//do not find the symbol, which should be impossible in my case.
}

//  PANEL HAS RESIZED, POSSIBLY FORGET SOME OLD OUTPUT LINES
void OutPanel::OnSizeEvent(wxSizeEvent& UNUSED(event))	// wxEVT_SIZE
{
    size_t new_nrows = GetSize().GetHeight() / fh;
    int	remove = lines.size() - new_nrows;

    while (remove > 0) {		// if panel has fewer rows, remove oldest lines
        lines.pop_front();
        --remove;
    }
    nrows = new_nrows;
}

//  PERIODICALLY CHECK FOR OUTPUT FROM gdb
void OutPanel::PollGDBOutput(wxTimerEvent& UNUSED(event))
{
    char buffer[8 * 1024], * bp;
    int  cc = read(fd_fromgdb, buffer, sizeof buffer);	// non-blocking

//  ANY CHARACTERS AVAILABLE?
    if (cc > 0) {
        bp = buffer;
        while (cc > 0) {
            if (*bp == '\n') {			// found end-of-line
                *lp = '\0';
                AddLine(line);
                lp = line;
                ++bp;
            }
            else {				// add to line
                *lp++ = *bp++;
            }
            --cc;
        }
        //  BUFFER EXHAUSTED, ANYTHING REMAINING IN line?
        if (lp > line) {
            *lp = '\0';
            AddLine(line);
            AddLine("");
            lp = line;

            //  gdb PROMPT FOUND => END OF gdb's OUTPUT
            if (strncmp(line, GDB_PROMPT, gdb_prompt_len) == 0) {
                timer->Stop();
            }
        }
    }
}
//  vim:set sw=4 ts=8: 
