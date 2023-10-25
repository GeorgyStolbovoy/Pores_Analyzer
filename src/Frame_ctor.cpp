#include "Frame.h"
#include <wx/sizer.h>

Frame::Frame() : wxFrame(nullptr, wxID_ANY, "Pores Analyzer")
{
	wxInitAllImageHandlers();

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_image = new ImageWindow( this );

	bSizer1->Add( m_image, 1, wxALL|wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	bSizer5->SetMinSize( wxSize( 400,300 ) );
	m_curves = new CorrectionWindow(this);

	bSizer5->Add( m_curves, 1, wxALL, 5 );

	m_slider2 = new wxSlider( this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LEFT|wxSL_VERTICAL );
	bSizer5->Add( m_slider2, 0, wxALL|wxEXPAND, 5 );


	bSizer1->Add( bSizer5, 0, wxEXPAND, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_checkBox1 = new wxCheckBox( this, wxID_ANY, wxT("Binary"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_checkBox1, 0, wxALL|wxEXPAND, 5 );

	m_slider1 = new wxSlider( this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_TOP );
	bSizer4->Add( m_slider1, 1, wxALL, 5 );


	bSizer1->Add( bSizer4, 0, wxEXPAND, 5 );


	SetSizer( bSizer1 );
	CreateStatusBar();
    SetStatusText("Welcome to wxWidgets!");
    SetSizeHints(1024, 768);
	Layout();
	Centre( wxBOTH );
	SetIcon({wxT("wxICON_AAA")});
}