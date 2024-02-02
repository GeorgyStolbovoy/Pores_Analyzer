#include "MeasureWindow.h"
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>

MeasureWindow::MeasureWindow(wxWindow *parent) : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize{350, -1}, wxBORDER_SUNKEN)
{
	wxBoxSizer* bSizer_measure = new wxBoxSizer( wxVERTICAL );

	//bSizer_measure->Add( 0, 0, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_checkBox_colorize = new wxCheckBox( this, checkbox_color_id, wxT("Выделить поры цветом"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBox_colorize->SetValue(true);
	bSizer5->Add( m_checkBox_colorize, 0, wxALL, 5 );

	m_button_changeColor = new wxButton( this, button_color_id, wxT("Поменять цвет"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_button_changeColor, 1, wxALL, 5 );

	bSizer_measure->Add( bSizer5, 0, wxEXPAND, 5 );

	m_checkBox_background = new wxCheckBox(this, checkbox_background_id, wxT("Автоудаление фона (самого большого элемента)"));
	m_checkBox_background->SetValue(true);
	bSizer_measure->Add( m_checkBox_background, 0, wxALL, 5 );

	wxStaticBoxSizer* static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Пороговая разность")), wxVERTICAL);
	m_slider_algorithm = new wxSlider( this, slider_algorithm_id, 128, 1, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	static_box_sizer->Add(m_slider_algorithm, 0, wxALL|wxEXPAND, 5);
	bSizer_measure->Add(static_box_sizer, 0, wxEXPAND, 5 );

	static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Прозрачность")), wxVERTICAL);
	m_slider_transparency = new wxSlider( this, slider_transparency_id, 0, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	static_box_sizer->Add(m_slider_transparency, 0, wxALL|wxEXPAND, 5);
	bSizer_measure->Add(static_box_sizer, 0, wxEXPAND, 5);

	static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Морфология")), wxVERTICAL);
	wxBoxSizer* sizer_morphology = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* sizer_erosion = new wxBoxSizer( wxVERTICAL );

	m_window_erosion = new MorphologyWindow(this);
	sizer_erosion->Add( m_window_erosion, 1, wxALL|wxEXPAND, 5 );

	m_button_erosion = new wxButton( this, button_erosion_id, wxT("Эрозия"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer_erosion->Add( m_button_erosion, 0, wxALL|wxEXPAND, 5 );

	sizer_morphology->Add( sizer_erosion, 1, wxEXPAND, 5 );

	wxBoxSizer* sizer_dilation = new wxBoxSizer( wxVERTICAL );

	m_window_dilation = new MorphologyWindow(this);
	sizer_dilation->Add( m_window_dilation, 1, wxALL|wxEXPAND, 5 );

	m_button_dilation = new wxButton( this, button_dilation_id, wxT("Наращивание"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer_dilation->Add( m_button_dilation, 0, wxALL|wxEXPAND, 5 );

	sizer_morphology->Add( sizer_dilation, 1, wxEXPAND, 5 );

	static_box_sizer->Add( sizer_morphology, 1, wxEXPAND, 5 );

	bSizer_measure->Add( static_box_sizer, 0, wxEXPAND, 5 );

	//bSizer_measure->Add( 0, 0, 1, wxEXPAND, 5 );

	SetSizer(bSizer_measure);
	Layout();
}