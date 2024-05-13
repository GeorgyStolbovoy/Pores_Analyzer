#pragma once

#include "ImageWindow.h"
#include "StatisticWindow.h"
#include "DoubleSlider.hpp"
#include "Utils.h"
#include <wx/slider.h>
#include <wx/tglbtn.h>
#include <wx/collpane.h>
#include <wx/sizer.h>
#include <set>
#include <unordered_set>
#include <random>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/container_hash/hash.hpp>

namespace mi = boost::multi_index;

struct MeasureWindow : wxWindow
{
	using coord_t = std::pair<std::ptrdiff_t, std::ptrdiff_t>;

private:
	using pore_coord_t = std::pair<uint32_t, coord_t>;
	using locator_t = ImageWindow::Image_t::view_t::xy_locator;
	using inspecting_pixel = std::pair<locator_t, coord_t>;
	struct pore_comp_t {bool operator()(const coord_t& lhs, const coord_t& rhs) const {return lhs.first == rhs.first && lhs.second == rhs.second;}};
	
	template <uint8_t ParamNumber, bool is_min_or_max>
	struct FilterCallback {void operator()(float min_value, float max_value);};

#define FILTER_CALLBACK(z, data, p) FilterCallback<StatisticWindow::pores_statistic_list_t::p, BOOST_PP_IF(data, true, false)>
#define FILTER_CALLBACK_SIZE(z, data, p) sizeof(FILTER_CALLBACK(~, data, p))
	using filter_callback_min_t = Invoker<std::max({BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(FILTER_CALLBACK_SIZE, 1, PORES_CALCULATING_PARAMS))}), void, float, float>;
	using filter_callback_max_t = Invoker<std::max({BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(FILTER_CALLBACK_SIZE, 0, PORES_CALCULATING_PARAMS))}), void, float, float>;
#undef FILTER_CALLBACK_SIZE
public:
	struct tag_multiset{}; struct tag_hashset{};
	using pores_container = boost::multi_index_container<
		pore_coord_t,
		mi::indexed_by<
			mi::ordered_non_unique<mi::tag<tag_multiset>, mi::key<&pore_coord_t::first>>,
			mi::hashed_unique<mi::tag<tag_hashset>, mi::key<&pore_coord_t::second>, boost::hash<coord_t>, pore_comp_t>
		>
	>;
	using double_slider_t = DoubleSlider<filter_callback_min_t, filter_callback_max_t>;

#define NAME_SLIDER(z, data, s) m_dslider_##s
#define ID_SLIDER(z, data, s) dslider_##s##_id
#define SLIDERS BOOST_PP_SEQ_TRANSFORM(NAME_SLIDER, ~, PORES_CALCULATING_PARAMS)
#define SLIDERS_IDS BOOST_PP_SEQ_TRANSFORM(ID_SLIDER, ~, PORES_CALCULATING_PARAMS)
	static wxWindowID collapse_segmentation_id, collapse_color_id, collapse_filter_id, slider_amount_id, slider_transparency_id, slider_thres_id,
		button_color_id, toggle_color_id, toggle_background_id, BOOST_PP_SEQ_ENUM(SLIDERS_IDS);
	wxSlider *m_slider_amount, *m_slider_transparency, *m_slider_thres;
#define ADD_POINTER(z, data, s) *s
	double_slider_t BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(ADD_POINTER, ~, SLIDERS));
#undef ADD_POINTER
	wxToggleButton *m_toggle_colorize;
	wxButton *m_button_changeColor;

	pores_container m_pores;
	std::set<uint32_t> m_deleted_pores, m_filtered_pores, m_selected_pores;
	std::unordered_set<coord_t, boost::hash<coord_t>> m_boundary_pixels;
	std::vector<uint8_t> m_colors;
	std::ptrdiff_t height = 0, width = 0;
	uint32_t pores_count;

	MeasureWindow();
	void NewMeasure(ImageWindow::Image_t::view_t view);
	void OnChangeThreshold(wxScrollEvent& event);
	void OnChangeSeparationAmount(wxScrollEvent& event);
	void OnChangeTransparency(wxScrollEvent& event);
	void OnChangeColor(wxCommandEvent& event);
	void OnSwitchColor(wxCommandEvent& event);
	void OnCollapse(wxCollapsiblePaneEvent& event);

private:
	std::mt19937 m_rand_engine{std::random_device{}()};
    std::uniform_int_distribution<uint16_t> m_random{0, 255};
	locator_t::cached_location_t cl_lt, cl_t, cl_rt, cl_r, cl_rb, cl_b, cl_lb, cl_l;
	uint8_t threshold = 100, amount = 50;

	void Segmentation();
	void after_measure();

	wxSizer *collapses_sizer, *pane_sizer;

	wxDECLARE_EVENT_TABLE();
};