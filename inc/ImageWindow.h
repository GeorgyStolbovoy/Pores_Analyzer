#pragma once

#include <wx/window.h>
#include <wx/bitmap.h>
#include <wx/timer.h>
#include <optional>
#include <boost/gil.hpp>

namespace gil = boost::gil;

struct ImageWindow : wxWindow
{
	using Image_t = gil::image<gil::gray8_pixel_t>;
	using Histogram_t = std::array<uint8_t, 256>;

	Image_t image;
	wxImage marked_image;
	Histogram_t histogram;
	enum class State : uint8_t {SCALING, SELECTING, DELETING, RECOVERING} state = State::SCALING;
	double scale_ratio;
	wxPoint scale_center, mouse_last_pos;

	ImageWindow(wxWindow *parent, const wxSize& size);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseLeftUp(wxMouseEvent& event);
	void OnMouseRightDown(wxMouseEvent& event);
	void OnMouseRightUp(wxMouseEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);
	void Load(const wxString& path);
	void ApplyHistogram(Histogram_t& hist);

	wxDECLARE_EVENT_TABLE();

private:
	Image_t image_source;
	wxBitmap DCbuffer;

	struct SelectionSession
	{
		wxTimer timer;
		unsigned char *selected_pores, *alpha;
		SelectionSession(ImageWindow* handler, std::size_t bufsize) : timer(handler), selected_pores(new unsigned char[bufsize*3]), alpha(new unsigned char[bufsize]{})
		{
			timer.Start(200);
		}
		~SelectionSession() {delete[] selected_pores; delete[] alpha;}
	};
	std::optional<SelectionSession> m_sel_session;
};