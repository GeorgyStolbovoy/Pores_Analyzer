#include "MeasureWindow.h"
#include "Frame.h"
#include "Utils.h"
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

MeasureWindow::MeasureWindow() : wxWindow(Frame::frame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN)
{
	wxBoxSizer* sizer_measure = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* sizer_horizontal = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Пороговая разность")), wxHORIZONTAL);
	m_slider_algorithm = new wxSlider( this, slider_algorithm_id, 128, 1, 255, wxDefaultPosition, wxSize(500, -1), wxSL_HORIZONTAL|wxSL_LABELS );
	static_box_sizer->Add(m_slider_algorithm, 0, wxALL|wxEXPAND|wxCENTRE, 5);
	sizer_horizontal->Add(static_box_sizer, 1, wxEXPAND, 5 );

	wxBoxSizer* sizer_vertical = new wxBoxSizer( wxVERTICAL );

	m_toggle_background = new wxToggleButton(this, toggle_background_id, wxT("Автоудаление фона (самого большого элемента)"));
	m_toggle_background->SetValue(true);
	sizer_vertical->Add( m_toggle_background, 0, wxALL, 5 );

	static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Метрика")), wxHORIZONTAL);

	wxString m_choice2Choices[]{wxT("(Не указывать)"), wxT("Длина"), wxT("Ширина")};
	wxChoice* m_choice2 = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, std::size(m_choice2Choices), m_choice2Choices);
	m_choice2->SetSelection(0);
	static_box_sizer->Add(m_choice2, 0, wxALL, 5);

	wxSpinCtrlDouble* m_spinCtrlDouble1 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1e-05, 100, 1, 0.1 );
	m_spinCtrlDouble1->SetDigits( 5 );
	static_box_sizer->Add( m_spinCtrlDouble1, 1, wxALL, 5 );

	wxString m_choice3Choices[]{wxT("м"), wxT("дм"), wxT("см"), wxT("мм"), wxT("мкм"), wxT("нм"), wxT("пм")};;
	wxChoice* m_choice3 = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, std::size(m_choice3Choices), m_choice3Choices);
	m_choice3->SetSelection( 0 );
	static_box_sizer->Add( m_choice3, 0, wxALL, 5 );

	sizer_vertical->Add( static_box_sizer, 1, wxEXPAND, 5 );

	sizer_horizontal->Add(sizer_vertical, 0, wxEXPAND, 5);

	sizer_measure->Add( sizer_horizontal, 0, wxEXPAND, 5 );

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

	sizer_horizontal = new wxBoxSizer(wxHORIZONTAL);

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

#define CALLBACK(z, data, p) FilterCallback<StatisticWindow::p, BOOST_PP_IF(data, true, false)>

	using callback_t = Invoker<std::max({1}), void, float>;

#define ADD_SLIDER(z, data, i, s) \
	BOOST_PP_EXPR_IF(BOOST_PP_NOT(BOOST_PP_MOD(i, 4)), sizer_horizontal = new wxBoxSizer(wxHORIZONTAL);) \
	static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(win_pane_filter, wxID_ANY, wxT(BOOST_PP_SEQ_ELEM(BOOST_PP_ADD(i, data), BOOST_PP_SEQ_POP_FRONT(PORES_PARAMS_NAMES)))), wxVERTICAL); \
	s = new DoubleSlider(win_pane_filter, callback_t{CALLBACK(~, 1, BOOST_PP_SEQ_ELEM(BOOST_PP_ADD(i, data), BOOST_PP_SEQ_POP_FRONT(PORES_PARAMS)))}, callback_t{CALLBACK(~, 0, BOOST_PP_SEQ_ELEM(BOOST_PP_ADD(i, data), BOOST_PP_SEQ_POP_FRONT(PORES_PARAMS)))}); \
	static_box_sizer->Add(s, 1, wxALL|wxEXPAND, 5); \
	sizer_horizontal->Add(static_box_sizer, data, wxEXPAND, 5); \
	BOOST_PP_EXPR_IF(BOOST_PP_NOT(BOOST_PP_MOD(i, 4)), paneSz->Add(sizer_horizontal, 1, wxEXPAND, 5);)

	ADD_SLIDER(~, 0, 0, BOOST_PP_SEQ_HEAD(SLIDERS))
	win_pane_filter->Bind(wxEVT_SIZE, [this, win_pane_filter, static_box_sizer](wxSizeEvent& e)
	{
		static_box_sizer->SetDimension(static_box_sizer->GetPosition(), {win_pane_filter->GetSize().GetWidth()/4, static_box_sizer->GetSize().GetHeight()});
	});
	BOOST_PP_SEQ_FOR_EACH_I(ADD_SLIDER, 1, BOOST_PP_SEQ_POP_FRONT(SLIDERS))

	win_pane_filter->SetSizer(paneSz);
	paneSz->SetSizeHints(win_pane_filter);

	collapses_sizer->Add(collpane_filter, 1, wxEXPAND, 5);

	sizer_measure->Add(collapses_sizer, 0, wxEXPAND, 5);

	pane_sizer = new wxBoxSizer(wxHORIZONTAL);

	sizer_measure->Add(pane_sizer, 0, wxEXPAND, 5);

	SetSizer(sizer_measure);
	Layout();
}