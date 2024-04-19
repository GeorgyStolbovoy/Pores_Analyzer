#pragma once

#include "Utils.h"
#include <wx/window.h>
#include <wx/listctrl.h>
#include <wx/checkbox.h>
#include <wx/aui/aui.h>
#include <wx/graphics.h>
#include <array>
#include <boost/preprocessor.hpp>

class StatisticWindow : public wxWindow
{
	struct Aui : wxAuiManager
	{
		Aui(wxWindow* parent) : wxAuiManager(parent, wxAUI_MGR_TRANSPARENT_HINT | wxAUI_MGR_HINT_FADE | wxAUI_MGR_NO_VENETIAN_BLINDS_FADE | wxAUI_MGR_LIVE_RESIZE) {}
        void SetPanesPositions(wxSizeEvent& e);
		template <auto EvtHandler>
		void SavePanePosition(wxMouseEvent& event);
		void OnPaneMinimaze(wxAuiManagerEvent& e);

	private:
		int sashSize = m_art->GetMetric(wxAUI_DOCKART_SASH_SIZE);
		std::pair<float, float> panes_positions{0.5f, 0.5f};

		wxDECLARE_EVENT_TABLE();
	};

	struct CommonStatisticList : wxListCtrl
	{
		using row_t = std::tuple<wxString, float>;
		enum : uint8_t {PARAM_NAME, PARAM_VALUE};
		std::array<row_t, 10> container;

		CommonStatisticList(StatisticWindow* parent);
		wxString OnGetItemText(long item, long column) const override;

	private:
		StatisticWindow* parent_statwindow;
	};

	struct PoresStatisticList : wxListCtrl
	{
#define PORES_PARAMS ID, SQUARE, BRIGHTNESS, PERIMETER, DIAMETER, CENTROID_X, CENTROID_Y, LENGTH, WIDTH, LENGTH_OY, WIDTH_OX, MAX_DIAMETER, MIN_DIAMETER, SHAPE, ELONGATION
#define STRINGSIZE_NAME(z, data, name) #name
#define PORES_PARAMS_NAMES BOOST_PP_SEQ_TRANSFORM(STRINGSIZE_NAME, ~, BOOST_PP_VARIADIC_TO_SEQ( \
			№, Площадь, Яркость, Периметр, Эквивалентный диаметр, Координата X, Координата Y, Длина, Ширина, Высота проекции, Ширина проекции, Наибольший диаметр, Наименьший диаметр, Фактор формы, Удлинённость))
		static_assert(BOOST_PP_VARIADIC_SIZE(PORES_PARAMS) == BOOST_PP_SEQ_SIZE(PORES_PARAMS_NAMES));
#define PORES_CALCULATING_PARAMS BOOST_PP_SEQ_POP_FRONT(BOOST_PP_VARIADIC_TO_SEQ(PORES_PARAMS))
#define FILL(z, data, p) data
		using row_t = std::tuple<uint32_t, BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(FILL, float, PORES_CALCULATING_PARAMS))>;
#undef PARAMS_TYPE
		using container_t = std::vector<row_t>;
		enum : uint8_t {PORES_PARAMS};
		container_t container;
		std::unordered_map<uint32_t, wxItemAttr> attributes;
		bool dont_affect = false;

		PoresStatisticList(StatisticWindow* parent);
		wxString OnGetItemText(long item, long column) const override;
		wxItemAttr* OnGetItemAttr(long item) const override;
		void OnSelection(wxListEvent& event);
		void OnDeselection(wxListEvent& event);
		void OnColumnClick(wxListEvent& event);
		void select_item(uint32_t pore_id);
		void deselect_item(uint32_t pore_id);
		void deselect_all();
		void on_pore_deleted(uint32_t pore_id);
		void on_pore_recovered(uint32_t pore_id);
		std::size_t item_to_index(long item) const;
		long index_to_item(std::size_t index);
		void set_item_count();
		void set_columns_width();

#define INDEX_TO_ID(item) (item - 3)
#define ID_TO_INDEX(id) (id + 3)
#define CONVERTER_BODY(arg, is_item_to_index, consider_deleted, consider_filtered) \
			BOOST_PP_IF(is_item_to_index, \
				BOOST_PP_EXPR_IF(consider_deleted, auto del_bound_it = Frame::frame->m_measure->m_deleted_pores.begin(); auto del_bound_end = Frame::frame->m_measure->m_deleted_pores.end();) \
				BOOST_PP_EXPR_IF(consider_filtered, auto fil_bound_it = Frame::frame->m_measure->m_filtered_pores.begin(); auto fil_bound_end = Frame::frame->m_measure->m_filtered_pores.end();) \
				for (std::ptrdiff_t dropped = 0;; dropped = 0) \
				{ \
					BOOST_PP_EXPR_IF(consider_deleted, \
						auto del_dropped_it = std::upper_bound(del_bound_it, del_bound_end, std::get<StatisticWindow::pores_statistic_list_t::ID>(Frame::frame->m_statistic->pores_statistic_list->container[arg])); \
						dropped = std::distance(std::exchange(del_bound_it, del_dropped_it), del_dropped_it); \
					) \
					BOOST_PP_EXPR_IF(consider_filtered, \
						auto fil_dropped_it = std::upper_bound(fil_bound_it, fil_bound_end, std::get<StatisticWindow::pores_statistic_list_t::ID>(Frame::frame->m_statistic->pores_statistic_list->container[arg])); \
						dropped += std::distance(std::exchange(fil_bound_it, fil_dropped_it), fil_dropped_it); \
					) \
					if (dropped == 0) \
						return arg; \
					else \
						arg += dropped; \
				} \
			, \
				std::ptrdiff_t dropped = 0; \
				BOOST_PP_EXPR_IF(consider_deleted, \
					dropped = std::distance(Frame::frame->m_measure->m_deleted_pores.begin(), Frame::frame->m_measure->m_deleted_pores.upper_bound(std::get<StatisticWindow::pores_statistic_list_t::ID>(Frame::frame->m_statistic->pores_statistic_list->container[arg]))); \
				) \
				BOOST_PP_EXPR_IF(consider_filtered, \
					dropped += std::distance(Frame::frame->m_measure->m_filtered_pores.begin(), Frame::frame->m_measure->m_filtered_pores.upper_bound(std::get<StatisticWindow::pores_statistic_list_t::ID>(Frame::frame->m_statistic->pores_statistic_list->container[arg]))); \
				) \
				return arg - dropped; \
			)
#define GET_CONVERTER(is_item_to_index) GET_STATIC_SWITCHER((std::conditional_t<is_item_to_index, std::size_t, long>), (std::conditional_t<is_item_to_index, long, std::size_t>), CONVERTER_BODY, (is_item_to_index), \
			(!Frame::frame->m_statistic->settings_window->m_checkBox_deleted->IsChecked() && !Frame::frame->m_measure->m_deleted_pores.empty()), (!Frame::frame->m_statistic->settings_window->m_checkBox_filtered->IsChecked() && !Frame::frame->m_measure->m_filtered_pores.empty()))

	private:
		StatisticWindow* parent_statwindow;

		void after_changes(float pores_square);

		wxDECLARE_EVENT_TABLE();
	};

	struct DistributionWindow : wxWindow
	{
		StatisticWindow* parent_statwindow;
		int column_index = 0;

		DistributionWindow(StatisticWindow* parent);
		void OnPaint(wxPaintEvent& event);
		void set_correct_font_size(wxGraphicsContext* gc, const wxString& text, int initial_size, double max_width, double max_height);

	private:
		wxDECLARE_EVENT_TABLE();
	};

	struct SettingsWindow : wxWindow
	{
		wxCheckBox *m_checkBox_color, *m_checkBox_deleted, *m_checkBox_filtered;
		static wxWindowID checkBox_color_id, checkBox_deleted_id, checkBox_filtered_id;

		SettingsWindow(wxWindow* parent);
		void OnDistributionColor(wxCommandEvent& event);
		void OnShowDeleted(wxCommandEvent& event);
		void OnShowFiltered(wxCommandEvent& event);

	private:
		StatisticWindow* parent_statwindow;

		wxDECLARE_EVENT_TABLE();
	};

	wxDECLARE_EVENT_TABLE();

public:
	using pores_statistic_list_t = PoresStatisticList;
	using common_statistic_list_t = CommonStatisticList;

	Aui* m_aui;
	CommonStatisticList* common_statistic_list;
	PoresStatisticList* pores_statistic_list;
	DistributionWindow* distribution_window;
	SettingsWindow* settings_window;

	uint32_t num_considered = 0;

	StatisticWindow();
	~StatisticWindow() {delete m_aui;}
	void collect_statistic_for_pore(uint32_t id);
	void CollectStatistic();
	void OnPaint(wxPaintEvent& event);
};