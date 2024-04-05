#include <wx/wxprec.h>
#include "Frame.h"

struct App : wxApp
{
	bool OnInit() override
	{
	    return (new Frame())->Show(true);
	}
};

wxIMPLEMENT_APP(App);