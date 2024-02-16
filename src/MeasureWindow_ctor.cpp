#include "MeasureWindow.h"
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>

MeasureWindow::MeasureWindow(wxWindow *parent) : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN)
{
	wxBoxSizer* sizer_measure = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* sizer_sliders = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* sizer_colors = new wxBoxSizer( wxHORIZONTAL );

	m_checkBox_colorize = new wxCheckBox( this, checkbox_color_id, wxT("Выделить поры цветом"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBox_colorize->SetValue(true);
	sizer_colors->Add( m_checkBox_colorize, 0, wxALL, 5 );

	m_button_changeColor = new wxButton( this, button_color_id, wxT("Поменять цвет"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer_colors->Add( m_button_changeColor, 1, wxALL, 5 );

	sizer_sliders->Add( sizer_colors, 0, wxEXPAND, 5 );

	m_checkBox_background = new wxCheckBox(this, checkbox_background_id, wxT("Автоудаление фона (самого большого элемента)"));
	m_checkBox_background->SetValue(true);
	sizer_sliders->Add( m_checkBox_background, 0, wxALL, 5 );

	wxStaticBoxSizer* static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Пороговая разность")), wxVERTICAL);
	m_slider_algorithm = new wxSlider( this, slider_algorithm_id, 128, 1, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	static_box_sizer->Add(m_slider_algorithm, 0, wxALL|wxEXPAND, 5);
	sizer_sliders->Add(static_box_sizer, 0, wxEXPAND, 5 );

	static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Прозрачность")), wxVERTICAL);
	m_slider_transparency = new wxSlider( this, slider_transparency_id, 0, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	static_box_sizer->Add(m_slider_transparency, 0, wxALL|wxEXPAND, 5);
	sizer_sliders->Add(static_box_sizer, 0, wxEXPAND, 5);

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

	sizer_sliders->Add( static_box_sizer, 1, wxEXPAND, 5 );

	sizer_measure->Add( sizer_sliders, 1, wxEXPAND, 5 );

	static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Морфология")), wxHORIZONTAL);

	wxBoxSizer* sizer_erosion = new wxBoxSizer( wxVERTICAL );

	m_window_erosion = new MorphologyWindow(this);
	sizer_erosion->Add( m_window_erosion, 1, wxALL|wxEXPAND, 5 );

	m_button_erosion = new wxButton( this, button_erosion_id, wxT("Эрозия"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer_erosion->Add( m_button_erosion, 0, wxALL|wxEXPAND, 5 );

	static_box_sizer->Add( sizer_erosion, 1, wxEXPAND, 5 );

	wxBoxSizer* sizer_dilation = new wxBoxSizer( wxVERTICAL );

	m_window_dilation = new MorphologyWindow(this);
	sizer_dilation->Add( m_window_dilation, 1, wxALL|wxEXPAND, 5 );

	m_button_dilation = new wxButton( this, button_dilation_id, wxT("Наращивание"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer_dilation->Add( m_button_dilation, 0, wxALL|wxEXPAND, 5 );

	static_box_sizer->Add( sizer_dilation, 1, wxEXPAND, 5 );

	sizer_measure->Add( static_box_sizer, 1, wxEXPAND, 5 );

	SetSizer(sizer_measure);
	Layout();
}