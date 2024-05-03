#include "MeasureWindow.h"
#include "Frame.h"
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

MeasureWindow::MeasureWindow() : wxWindow(Frame::frame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN)
{
	wxBoxSizer *sizer_measure = new wxBoxSizer( wxVERTICAL ), *sizer_horizontal = new wxBoxSizer( wxHORIZONTAL ), *sizer_vertical = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Порог")), wxHORIZONTAL);
	m_slider_thres = new wxSlider( this, slider_thres_id, threshold, 0, 255, wxDefaultPosition, wxSize(500, -1), wxSL_HORIZONTAL|wxSL_LABELS );
	static_box_sizer->Add(m_slider_thres, 1, wxALL|wxEXPAND|wxCENTRE, 5);
	sizer_vertical->Add(static_box_sizer, 1, wxEXPAND, 5 );

	static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Чувствительность поиска слипшихся пор")), wxHORIZONTAL);
	m_slider_amount = new wxSlider( this, slider_amount_id, amount, 2, 255, wxDefaultPosition, wxSize(500, -1), wxSL_HORIZONTAL|wxSL_LABELS );
	static_box_sizer->Add(m_slider_amount, 1, wxALL|wxEXPAND|wxCENTRE, 5);
	sizer_vertical->Add(static_box_sizer, 1, wxEXPAND, 5 );

	sizer_measure->Add( sizer_vertical, 0, wxEXPAND, 5 );

	collapses_sizer = new wxBoxSizer( wxHORIZONTAL );

	wxCollapsiblePane *collpane = new wxCollapsiblePane(this, collapse_morphology_id, "Морфология", wxDefaultPosition, wxDefaultSize, wxCP_NO_TLW_RESIZE);

	wxWindow *win = collpane->GetPane();
	wxSizer *paneSz = new wxBoxSizer(wxHORIZONTAL);

	paneSz->Add(0, 0, 1, wxEXPAND, 5);

	wxBoxSizer* sizer_erosion = new wxBoxSizer( wxVERTICAL );

	m_window_erosion = new MorphologyWindow(win);
	sizer_erosion->Add( m_window_erosion, 1, wxALL|wxEXPAND, 5 );

	m_button_erosion = new wxButton( win, button_erosion_id, wxT("Эрозия"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer_erosion->Add( m_button_erosion, 0, wxALL|wxEXPAND, 5 );

	paneSz->Add( sizer_erosion, 0, wxEXPAND, 5 );

	paneSz->Add(0, 0, 1, wxEXPAND, 5);

	wxBoxSizer* sizer_dilation = new wxBoxSizer( wxVERTICAL );

	m_window_dilation = new MorphologyWindow(win);
	sizer_dilation->Add( m_window_dilation, 1, wxALL|wxEXPAND, 5 );

	m_button_dilation = new wxButton( win, button_dilation_id, wxT("Наращивание"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer_dilation->Add( m_button_dilation, 0, wxALL|wxEXPAND, 5 );

	paneSz->Add( sizer_dilation, 0, wxEXPAND, 5 );

	paneSz->Add(0, 0, 1, wxEXPAND, 5);

	win->SetSizer(paneSz);
	paneSz->SetSizeHints(win);

	collapses_sizer->Add(collpane, 1, wxEXPAND, 5);

	wxCollapsiblePane *collpane_color = new wxCollapsiblePane(this, collapse_color_id, "Настройки цвета", wxDefaultPosition, wxDefaultSize, wxCP_NO_TLW_RESIZE);

	wxWindow *win_pane_color = collpane_color->GetPane();

	paneSz = new wxBoxSizer( wxVERTICAL );

	static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(win_pane_color, wxID_ANY, wxT("Границы пор")), wxVERTICAL);
	m_toggle_boundaries = new wxToggleButton(win_pane_color, toggle_boundaries_id, wxT("Выделить границы пор"));
	m_toggle_boundaries->SetValue(true);
	static_box_sizer->Add(m_toggle_boundaries, 0, wxALL|wxEXPAND, 5);

	wxSizer* sizer_horizontal_2 = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* static_text = new wxStaticText(win_pane_color, wxID_ANY, wxT("Цвет: "));
	sizer_horizontal_2->Add(static_text, 0, wxALIGN_CENTER_VERTICAL, 5);

	m_colorpicker_boundaries = new wxColourPickerCtrl(win_pane_color, wxID_ANY, *wxRED, wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_LABEL);
	sizer_horizontal_2->Add(m_colorpicker_boundaries, 1, wxEXPAND, 5);
	
	static_box_sizer->Add(sizer_horizontal_2, 0, wxALL|wxEXPAND, 5);
	
	sizer_horizontal->Add(static_box_sizer, 0, wxEXPAND, 5);

	static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(win_pane_color, wxID_ANY, wxT("Прозрачность")), wxVERTICAL);
	m_slider_transparency = new wxSlider( win_pane_color, slider_transparency_id, 0, 0, 100, wxDefaultPosition, wxDefaultSize /*wxSize(500, -1)*/, wxSL_HORIZONTAL|wxSL_LABELS );
	static_box_sizer->Add(m_slider_transparency, 1, wxALL|wxEXPAND, 5);
	
	sizer_horizontal->Add(static_box_sizer, 1, wxEXPAND, 5);

	paneSz->Add( sizer_horizontal, 1, wxEXPAND, 5 );

	sizer_horizontal = new wxBoxSizer(wxHORIZONTAL);

	m_toggle_colorize = new wxToggleButton(win_pane_color, toggle_color_id, wxT("Выделить поры цветом"));
	m_toggle_colorize->SetValue(true);
	sizer_horizontal->Add( m_toggle_colorize, 1, wxALL|wxEXPAND, 5 );

	m_button_changeColor = new wxButton( win_pane_color, button_color_id, wxT("Поменять цвет"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer_horizontal->Add( m_button_changeColor, 1, wxALL|wxEXPAND, 5 );

	paneSz->Add( sizer_horizontal, 0, wxEXPAND, 5 );

	win_pane_color->SetSizer(paneSz);
	paneSz->SetSizeHints(win_pane_color);

	collapses_sizer->Add(collpane_color, 1, wxEXPAND, 5);

	wxCollapsiblePane *collpane_filter = new wxCollapsiblePane(this, collapse_filter_id, "Фильтр", wxDefaultPosition, wxDefaultSize, wxCP_NO_TLW_RESIZE);

	wxWindow *win_pane_filter = collpane_filter->GetPane();

	paneSz = new wxBoxSizer( wxVERTICAL );

#define ADD_SLIDER(z, data, i, s) \
	BOOST_PP_EXPR_IF(BOOST_PP_NOT(BOOST_PP_MOD(i, 4)), sizer_horizontal = new wxBoxSizer(wxHORIZONTAL);) \
	static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(win_pane_filter, wxID_ANY, wxT(BOOST_PP_SEQ_ELEM(BOOST_PP_ADD(i, data), BOOST_PP_SEQ_POP_FRONT(PORES_PARAMS_NAMES)))), wxVERTICAL); \
	s = new DoubleSlider(win_pane_filter, filter_callback_min_t{FILTER_CALLBACK(~, 1, BOOST_PP_SEQ_ELEM(BOOST_PP_ADD(i, data), PORES_CALCULATING_PARAMS)){}}, filter_callback_max_t{FILTER_CALLBACK(~, 0, BOOST_PP_SEQ_ELEM(BOOST_PP_ADD(i, data), PORES_CALCULATING_PARAMS)){}}); \
	static_box_sizer->Add(s, 1, wxALL|wxEXPAND, 5); \
	sizer_horizontal->Add(static_box_sizer, 1, wxEXPAND, 5); \
	BOOST_PP_EXPR_IF(BOOST_PP_NOT(BOOST_PP_MOD(i, 4)), paneSz->Add(sizer_horizontal, 1, wxEXPAND, 5);)

	ADD_SLIDER(~, 0, 0, BOOST_PP_SEQ_HEAD(SLIDERS))
	ADD_SLIDER(~, 0, 1, BOOST_PP_SEQ_ELEM(1, SLIDERS))
	win_pane_filter->Bind(wxEVT_SIZE, [this, win_pane_filter, static_box_sizer](wxSizeEvent& e)
	{
		static_box_sizer->SetDimension(static_box_sizer->GetPosition(), {win_pane_filter->GetSize().GetWidth()/2, static_box_sizer->GetSize().GetHeight()});
	});
	BOOST_PP_SEQ_FOR_EACH_I(ADD_SLIDER, 2, BOOST_PP_SEQ_REST_N(2, SLIDERS))

	win_pane_filter->SetSizer(paneSz);
	paneSz->SetSizeHints(win_pane_filter);

	collapses_sizer->Add(collpane_filter, 1, wxEXPAND, 5);

	sizer_measure->Add(collapses_sizer, 0, wxEXPAND, 5);

	pane_sizer = new wxBoxSizer(wxHORIZONTAL);

	sizer_measure->Add(pane_sizer, 0, wxEXPAND, 5);

	sizer_measure->SetMinSize(paneSz->GetMinSize().GetWidth() + 4*wxSystemSettings::GetMetric(wxSystemMetric::wxSYS_BORDER_X), -1);
	SetSizer(sizer_measure);
	Layout();
}

template <uint8_t ParamNumber, bool is_min_or_max>
void MeasureWindow::FilterCallback<ParamNumber, is_min_or_max>::operator()(float min_value, float max_value)
{
	if (gil::view(Frame::frame->m_image->image).empty()) [[unlikely]]
		return;

#define RESET_MIN_MAX(z, _, p) std::get<StatisticWindow::pores_statistic_list_t::p>(Frame::frame->m_statistic->pores_statistic_list->container[(CONDITIONAL(is_min_or_max, 2, 3))]) \
			= CONDITIONAL(is_min_or_max, std::numeric_limits<float>::max(), std::numeric_limits<float>::min());
#define REFRESH_MIN_MAX_MEAN(z, tuple_is_add_refresh_mean, p) \
		{ \
			float pore_value = std::get<StatisticWindow::pores_statistic_list_t::p>(*it); \
			BOOST_PP_EXPR_IF(BOOST_PP_TUPLE_ELEM(0, tuple_is_add_refresh_mean), \
				float& extr_value = std::get<StatisticWindow::pores_statistic_list_t::p>(Frame::frame->m_statistic->pores_statistic_list->container[(CONDITIONAL(is_min_or_max, 2, 3))]); \
				if (CONDITIONAL(is_min_or_max, extr_value > pore_value, extr_value < pore_value, =)) [[unlikely]] \
					extr_value = pore_value; \
			) \
			BOOST_PP_EXPR_IF(BOOST_PP_TUPLE_ELEM(1, tuple_is_add_refresh_mean), \
				float& mean_value = std::get<StatisticWindow::pores_statistic_list_t::p>(Frame::frame->m_statistic->pores_statistic_list->container[0]); \
				mean_value = (mean_value * (Frame::frame->m_statistic->num_considered BOOST_PP_IF(BOOST_PP_TUPLE_ELEM(0, tuple_is_add_refresh_mean), -, +) 1) \
					BOOST_PP_IF(BOOST_PP_TUPLE_ELEM(0, tuple_is_add_refresh_mean), +, -) pore_value) / Frame::frame->m_statistic->num_considered; \
			) \
		}
#define ADD_OR_DEDUCT(is_add) \
		BOOST_PP_IF(is_add, ++, --)Frame::frame->m_statistic->num_considered; \
		pores_square BOOST_PP_IF(is_add, +=, -=) Frame::frame->m_measure->m_pores.get<tag_multiset>().count(id); \
		BOOST_PP_SEQ_FOR_EACH(REFRESH_MIN_MAX_MEAN, (is_add, 1), PORES_CALCULATING_PARAMS)
#define CHECK_FILTERS(z, iter_to_row, i, s) \
		if constexpr (ParamNumber != StatisticWindow::pores_statistic_list_t::BOOST_PP_SEQ_ELEM(i, PORES_CALCULATING_PARAMS)) \
		{ \
			float value = std::get<StatisticWindow::pores_statistic_list_t::BOOST_PP_SEQ_ELEM(i, PORES_CALCULATING_PARAMS)>(*iter_to_row); \
			if (auto [min_value, max_value] = Frame::frame->m_measure->s->get_values(); min_value > value || max_value < value) [[unlikely]] \
				return false; \
		}

	float &pores_square = std::get<StatisticWindow::common_statistic_list_t::PARAM_VALUE>(Frame::frame->m_statistic->common_statistic_list->container[4]);
	BOOST_PP_SEQ_FOR_EACH(RESET_MIN_MAX, ~, PORES_CALCULATING_PARAMS)
	for (auto it = Frame::frame->m_statistic->pores_statistic_list->container.begin() + 4, end = Frame::frame->m_statistic->pores_statistic_list->container.end(); it != end; ++it)
	{
		if (uint32_t id = std::get<StatisticWindow::pores_statistic_list_t::ID>(*it); !Frame::frame->m_measure->m_deleted_pores.contains(id))
		{
			if (float value = std::get<ParamNumber>(*it); CONDITIONAL(is_min_or_max, value < min_value, value > max_value, =))
			{
				if (Frame::frame->m_measure->m_filtered_pores.insert(id).second)
				{
					wxItemAttr attr;
					attr.SetBackgroundColour(0xAAEEEE);
					Frame::frame->m_statistic->pores_statistic_list->attributes.insert({id, attr});
					if (Frame::frame->m_measure->m_selected_pores.contains(id)) [[unlikely]]
						Frame::frame->m_image->deselect_pore(id);
					ADD_OR_DEDUCT(0)
				}
			}
			else if (auto find_it = Frame::frame->m_measure->m_filtered_pores.find(id); find_it != Frame::frame->m_measure->m_filtered_pores.end())
			{
				if (CONDITIONAL(is_min_or_max, value <= max_value, value >= min_value, =) && [it](){BOOST_PP_SEQ_FOR_EACH_I(CHECK_FILTERS, it, SLIDERS) return true;}())
				{
					Frame::frame->m_measure->m_filtered_pores.erase(find_it);
					Frame::frame->m_statistic->pores_statistic_list->attributes.erase(id);
					ADD_OR_DEDUCT(1)
				}
			}
			else
			{
				BOOST_PP_SEQ_FOR_EACH(REFRESH_MIN_MAX_MEAN, (1, 0), PORES_CALCULATING_PARAMS)
			}
		}
	}
	Frame::frame->m_statistic->pores_statistic_list->set_item_count();

	uint32_t all_size = Frame::frame->m_measure->width * Frame::frame->m_measure->height;
	std::get<StatisticWindow::common_statistic_list_t::PARAM_VALUE>(Frame::frame->m_statistic->common_statistic_list->container[2]) = Frame::frame->m_statistic->num_considered;
	std::get<StatisticWindow::common_statistic_list_t::PARAM_VALUE>(Frame::frame->m_statistic->common_statistic_list->container[3]) = pores_square/float(all_size);
	std::get<StatisticWindow::common_statistic_list_t::PARAM_VALUE>(Frame::frame->m_statistic->common_statistic_list->container[5]) = all_size - pores_square;

#define SET_MEAN_DEVIATION(z, set_deviation, p) \
		{ \
			float mean = std::get<StatisticWindow::pores_statistic_list_t::p>(Frame::frame->m_statistic->pores_statistic_list->container[0]); \
			BOOST_PP_IF(set_deviation, \
				double deviation = 0; \
				for (auto it = Frame::frame->m_statistic->pores_statistic_list->container.begin() + 4, end = Frame::frame->m_statistic->pores_statistic_list->container.end(); it != end; ++it) \
					if (!Frame::frame->m_statistic->pores_statistic_list->attributes.contains(std::get<StatisticWindow::pores_statistic_list_t::ID>(*it))) \
						deviation += std::pow(mean - std::get<StatisticWindow::pores_statistic_list_t::p>(*it), 2); \
				std::get<StatisticWindow::pores_statistic_list_t::p>(Frame::frame->m_statistic->pores_statistic_list->container[1]) = std::sqrtf(deviation/(Frame::frame->m_statistic->num_considered-1)); \
				, \
				std::get<StatisticWindow::pores_statistic_list_t::p>(Frame::frame->m_statistic->pores_statistic_list->container[1]) = 1; \
			) \
		}

	if (Frame::frame->m_statistic->num_considered > 1) {
		BOOST_PP_SEQ_FOR_EACH(SET_MEAN_DEVIATION, 1, PORES_CALCULATING_PARAMS)
	} else if (Frame::frame->m_statistic->num_considered == 1) {
		BOOST_PP_SEQ_FOR_EACH(SET_MEAN_DEVIATION, 0, PORES_CALCULATING_PARAMS)
	}

	Frame::frame->m_image->marked_image = wxNullImage;
	Frame::frame->m_image->Refresh();
	Frame::frame->m_statistic->common_statistic_list->Refresh();
	Frame::frame->m_statistic->pores_statistic_list->Refresh();
	Frame::frame->m_statistic->distribution_window->Refresh();
	Frame::frame->m_image->Update();
	Frame::frame->m_statistic->distribution_window->Update();
	Frame::frame->m_statistic->common_statistic_list->Update();
	Frame::frame->m_statistic->pores_statistic_list->Update();
}