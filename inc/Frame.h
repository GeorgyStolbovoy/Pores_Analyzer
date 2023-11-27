#pragma once

#include "ImageWindow.h"
#ifndef VIEWER
#include "CorrectionWindow.h"
#endif
#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/statline.h>
#include <wx/socket.h>

class Frame : public wxFrame
{
    ImageWindow* m_image;
#ifndef VIEWER
	CorrectionWindow* m_curves;
#endif
	wxMenuItem* m_menuItem4;
	wxStaticLine* m_staticline1;

	wxDatagramSocket* sock;
	static wxWindowID soa_id, sock_id;

public:
    Frame();
	~Frame();
#ifndef VIEWER
	void RefreshImage(ImageWindow::Histogram_t&& hist);
#endif
	void Load_Image(wxCommandEvent& event);
	void OnSoa(wxCommandEvent& event);
#ifdef VIEWER
	void OnSocket(wxSocketEvent& event);
#endif
	void SoaCleanup();
	void Exit(wxCommandEvent& event) {Destroy();}

	wxDECLARE_EVENT_TABLE();
};