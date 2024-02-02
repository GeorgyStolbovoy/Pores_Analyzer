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
	void OnLoad(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnToolbarScale(wxCommandEvent& event);
	void OnToolbarSelect(wxCommandEvent& event);
	void OnToolbarDelete(wxCommandEvent& event);
	void OnToolbarRecovery(wxCommandEvent& event);
	template <class E> void Exit(E& event) {Destroy();}

	wxDECLARE_EVENT_TABLE();
};