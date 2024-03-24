#pragma once

#include <wx/window.h>
#include <wx/graphics.h>

struct DoubleSlider : wxWindow
{
	bool active = false;
	float min = 0.0f, max = 0.0f;

	DoubleSlider(wxWindow* parent);
	void set_values(float min_, float max_);
	std::pair<float, float> get_values();
	void OnPaint(wxPaintEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseLeftUp(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);
	void OnCaptureLost(wxMouseCaptureLostEvent& event) {capture = nullptr;}

private:
	float min_pos = 0.0f, max_pos = 1.0f;
	wxGraphicsPath min_slider_path, max_slider_path;
	wxGraphicsPath *hover = nullptr, *capture = nullptr, *foreground = &min_slider_path;
	int last_mx;
	static const wxString text_placeholder;

	template <bool format_number>
	std::pair<double, double> fit_text(wxGraphicsContext* gc, std::conditional_t<format_number, wxString, const wxString>& text, const double max_width);

	wxDECLARE_EVENT_TABLE();
};