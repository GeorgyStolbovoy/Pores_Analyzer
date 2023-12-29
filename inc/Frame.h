#pragma once

#include "ImageWindow.h"
#include "CorrectionWindow.h"
#include "MeasureWindow.h"
#include <wx/frame.h>

class Frame : public wxFrame
{
public:
    ImageWindow* m_image;
	CorrectionWindow* m_curves;
	MeasureWindow* m_measure;

    Frame();
	void RefreshImage(ImageWindow::Histogram_t& hist);
	void Load_Image(wxCommandEvent& event);
	void Exit(wxCommandEvent& event) {Destroy();}

	wxDECLARE_EVENT_TABLE();
};