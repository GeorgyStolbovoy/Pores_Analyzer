#pragma once

#include "ImageWindow.h"
#ifndef VIEWER
#include "CorrectionWindow.h"
#endif
#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/statline.h>
#include <boost/interprocess/managed_shared_memory.hpp>

class Frame : public wxFrame
{
    ImageWindow* m_image;
#ifndef VIEWER
	CorrectionWindow* m_curves;
#endif
	wxMenuItem* m_menuItem4;
	wxStaticLine* m_staticline1;

	boost::interprocess::managed_shared_memory segment;
	static wxWindowID soa_id;
	static constexpr const char
		*shared_memory_name = "pores_analyser",
		*shared_callback_name = "pores_analyser:callback",
		*shared_string_name = "pores_analyser:string",
		*shared_flag_name = "pores_analyser:flag";

public:
    Frame();
	~Frame();
#ifndef VIEWER
	void RefreshImage(ImageWindow::Histogram_t&& hist);
#endif
	void Load_Image(wxCommandEvent& event);
	void OnSoa(wxCommandEvent& event);
#ifdef VIEWER
	static void SoaCallback();
#endif
	void SoaCleanup();
	void Exit(wxCommandEvent& event) {Destroy();}

	wxDECLARE_EVENT_TABLE();
};