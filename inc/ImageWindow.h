#pragma once

#include <wx/window.h>
#include <wx/bitmap.h>
#include <boost/gil.hpp>

namespace gil = boost::gil;

class ImageWindow : public wxWindow
{
	wxBitmap DCbuffer;

public:
	using Histogram_t = std::unordered_map<uint8_t, uint8_t>;

	static gil::image<gil::gray8_pixel_t> image, image_current;
	Histogram_t histogram;

	ImageWindow(wxWindow *parent);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnSize();
	void ApplyHistogram();

	wxDECLARE_EVENT_TABLE();
};