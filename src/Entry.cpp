#include <wx/wxprec.h>
#include "Frame.h"

Frame* Frame::frame = new Frame();

struct App : wxApp
{
	bool OnInit() override
	{
	    return Frame::frame->Show(true);
	}
};

wxIMPLEMENT_APP(App);