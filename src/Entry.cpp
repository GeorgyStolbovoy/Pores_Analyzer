#include <wx/wxprec.h>
#include "Frame.h"

struct App : wxApp
{
	bool OnInit() override
	{
	    Frame *frame = new Frame();
	    frame->Show(true);
	    return true;
	}
};

wxIMPLEMENT_APP(App);