#include "MeasureWindow.h"
#include <wx/sizer.h>
#include <wx/button.h>

MeasureWindow::MeasureWindow(wxWindow *parent) : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize{350, -1}, wxBORDER_SUNKEN)
{
	wxBoxSizer* bSizer_measure = new wxBoxSizer( wxVERTICAL );

	//bSizer_measure->Add( 0, 0, 1, wxEXPAND, 5 );

	m_slider_algorithm = new wxSlider( this, slider_algorithm_id, 128, 1, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	bSizer_measure->Add( m_slider_algorithm, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_checkBox_colorize = new wxCheckBox( this, checkbox_color_id, wxT("Выделить поры цветом"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_checkBox_colorize, 0, wxALL, 5 );

	m_button_changeColor = new wxButton( this, button_color_id, wxT("Поменять цвет"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_button_changeColor, 1, wxALL, 5 );

	bSizer_measure->Add( bSizer5, 1, wxEXPAND, 5 );

	SetSizer(bSizer_measure);
	Layout();
}