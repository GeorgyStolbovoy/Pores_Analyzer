#pragma once

#include <wx/window.h>
#include <wx/graphics.h>

struct DoubleSlider : wxWindow
{
	bool active = false;
	float min = 0.0f, max = 0.0f;

	DoubleSlider(wxWindow* parent);
	void OnPaint(wxPaintEvent& event);

private:
	float min_pos = 0.0f, max_pos = 1.0f;
	wxGraphicsPath min_slider_path, max_slider_path;

	wxDECLARE_EVENT_TABLE();
};