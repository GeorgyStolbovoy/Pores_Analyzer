#pragma once

#include "ImageWindow.h"
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <set>
#include <random>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/container_hash/hash.hpp>

namespace mi = boost::multi_index;

// TODO : сменить хэш на что-то очень простое

class MeasureWindow : public wxWindow
{
	using coord_t = std::pair<std::ptrdiff_t, std::ptrdiff_t>;
	using pore_coord_t = std::pair<uint32_t, coord_t>;
	using locator_t = decltype(ImageWindow::image_current)::view_t::xy_locator;
	using inspecting_pixel = std::pair<locator_t, coord_t>;
	struct pore_comp_t {bool operator()(const coord_t& lhs, const coord_t& rhs) const {return lhs.first == rhs.first && lhs.second == rhs.second;}};

public:
	static wxWindowID slider_algorithm_id, slider_transparency_id, button_color_id, button_erosion_id, button_dilation_id, checkbox_color_id, checkbox_background_id;
	wxSlider *m_slider_algorithm, *m_slider_transparency;
	wxCheckBox *m_checkBox_colorize, *m_checkBox_background;
	wxButton *m_button_changeColor, *m_button_erosion, *m_button_dilation;

	struct tag_multiset{}; struct tag_hashset{}; struct tag_sequenced{};
	boost::multi_index_container<
		pore_coord_t,
		mi::indexed_by<
			mi::ordered_non_unique<mi::tag<tag_multiset>, mi::key<&pore_coord_t::first>>,
			mi::hashed_unique<mi::tag<tag_hashset>, mi::key<&pore_coord_t::second>, boost::hash<coord_t>, pore_comp_t>
		>
	> m_pores;
	std::set<uint32_t> m_deleted_pores;
	std::vector<uint8_t> m_colors;

	MeasureWindow(wxWindow *parent);
	void NewMeasure();
	void OnChangeDifference(wxScrollEvent& event);
	void OnChangeTransparency(wxScrollEvent& event);
	void OnChangeColor(wxCommandEvent& event);
	void OnErosion(wxCommandEvent& event);
	void OnDilation(wxCommandEvent& event);
	void OnSwitchColor(wxCommandEvent& event);
	void OnDeleteBackground(wxCommandEvent& event);

private:
	using inspecting_pixels_t = boost::multi_index_container<
		inspecting_pixel,
		mi::indexed_by<
			mi::sequenced<mi::tag<tag_sequenced>>,
			mi::hashed_unique<mi::tag<tag_hashset>, mi::key<&inspecting_pixel::second>, boost::hash<coord_t>, pore_comp_t>
		>
	>;

	inspecting_pixels_t m_checklist;
	std::mt19937 m_rand_engine{std::random_device{}()};
    std::uniform_int_distribution<uint16_t> m_random{0, 255};
	locator_t::cached_location_t cl_lt, cl_t, cl_rt, cl_r, cl_rb, cl_b, cl_lb, cl_l;
	std::ptrdiff_t height, width;
	uint32_t pores_count;
	uint8_t diff;

	void Measure(locator_t&& loc = gil::view(ImageWindow::image_current).xy_at(0, 0));
	void inspect_pore(const inspecting_pixel& insp_pixel);
	uint32_t get_biggest_pore_id();

	wxDECLARE_EVENT_TABLE();
};