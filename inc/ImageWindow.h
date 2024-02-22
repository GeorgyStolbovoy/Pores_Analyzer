#pragma once

#include <wx/window.h>
#include <wx/bitmap.h>
#include <wx/timer.h>
#include <optional>
#include <boost/gil.hpp>

namespace gil = boost::gil;
class Frame;
struct MeasureWindow;

struct ImageWindow : wxWindow
{
	using Image_t = gil::image<gil::gray8_pixel_t>;
	using Histogram_t = std::array<uint8_t, 256>;
	struct SelectionSession : wxTimer
	{
		std::size_t bufsize;
		unsigned char *selected_pores, *alpha;
		float rad = float(M_PI);
		SelectionSession(ImageWindow* owner, std::size_t bufsize);
		void deselect(MeasureWindow* measure, uint32_t pore_id, std::ptrdiff_t row_width);
		~SelectionSession() {delete[] selected_pores; delete[] alpha;}
	};
	struct RecoveringBackground
	{
		unsigned char *background, *alpha;
		RecoveringBackground(std::size_t bufsize);
		void recover(MeasureWindow* measure, uint32_t pore_id, std::ptrdiff_t row_width);
		~RecoveringBackground() {delete[] background; delete[] alpha;}
	};

	Image_t image;
	wxImage marked_image;
	Histogram_t histogram;
	enum class State : uint8_t {SCALING, SELECTING, DELETING, RECOVERING} state = State::SCALING;
	double scale_ratio;
	wxPoint scale_center, mouse_last_pos;
	std::optional<SelectionSession> m_sel_session;
	std::optional<RecoveringBackground> m_rec_background;

	ImageWindow(wxWindow *parent);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseLeftUp(wxMouseEvent& event);
	void OnMouseRightDown(wxMouseEvent& event);
	void OnMouseRightUp(wxMouseEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnTimer(wxTimerEvent& event);
	void Load(const wxString& path);
	void ApplyHistogram(Histogram_t& hist);

private:
	Frame* parent_frame;
	Image_t image_source;
	wxBitmap DCbuffer;

	wxDECLARE_EVENT_TABLE();
};