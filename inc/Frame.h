#pragma once

#include "ImageWindow.h"
#include "CorrectionWindow.h"
#include "MeasureWindow.h"
#include "StatisticWindow.h"
#include <wx/frame.h>
#include <wx/toolbar.h>

class Frame : public wxFrame
{
public:
    ImageWindow* m_image;
	CorrectionWindow* m_curves;
	MeasureWindow* m_measure;
	StatisticWindow* m_statistic;

	wxToolBar* m_toolBar;

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