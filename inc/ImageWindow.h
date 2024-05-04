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
	struct SelectionSession : wxTimer
	{
		std::size_t bufsize;
		unsigned char *selected_pores, *alpha;
		float rad = float(M_PI);

		SelectionSession(ImageWindow* owner);
		SelectionSession(SelectionSession& other) = delete;
		void select(uint32_t pore_id);
		void deselect(uint32_t pore_id);
		~SelectionSession() {delete[] selected_pores; delete[] alpha;}
	};
	struct RecoveringBackground
	{
		unsigned char *background, *alpha;
		RecoveringBackground(std::size_t bufsize);
		void recover(uint32_t pore_id);
		~RecoveringBackground() {delete[] background; delete[] alpha;}
	};

	Image_t image, image_source;
	wxImage marked_image;
	Histogram_t histogram;
	enum class State : uint8_t {SCALING, SELECTING, DELETING, RECOVERING} state = State::SCALING;
	double scale_ratio;
	wxPoint scale_center, mouse_last_pos;
	std::optional<SelectionSession> m_sel_session;
	std::optional<RecoveringBackground> m_rec_background;

	ImageWindow();
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseLeftUp(wxMouseEvent& event);
	void OnMouseRightDown(wxMouseEvent& event);
	void OnMouseRightUp(wxMouseEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnCaptureLost(wxMouseCaptureLostEvent& event) {}
	void OnTimer(wxTimerEvent& event);
	void Load(const wxString& path);
	void ApplyHistogram(Histogram_t& hist);
	void deselect_pore(uint32_t id);
	void update_image(bool reset_selection);

private:
	wxBitmap DCbuffer;

	wxDECLARE_EVENT_TABLE();
};