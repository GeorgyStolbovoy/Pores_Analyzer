#pragma once

#include <wx/window.h>
#include <wx/bitmap.h>
#include <boost/gil.hpp>

namespace gil = boost::gil;

class ImageWindow : public wxWindow
{
	wxBitmap DCbuffer;

public:
	using Histogram_t = std::array<uint8_t, 256>;

	static gil::image<gil::gray8_pixel_t> image, image_current;
	Histogram_t histogram;

	ImageWindow(wxWindow *parent, const wxSize& size);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnSizeLinear();

	wxDECLARE_EVENT_TABLE();
};