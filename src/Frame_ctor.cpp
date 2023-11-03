#include "Frame.h"
#include <wx/sizer.h>

Frame::Frame() : wxFrame(nullptr, wxID_ANY, "Pores Analyzer")
{
	wxMenuBar* m_menubar1 = new wxMenuBar(0);
	wxMenu* m_menu1 = new wxMenu();
	wxMenuItem* m_menuItem1 = new wxMenuItem( m_menu1, wxID_OPEN, wxString(wxT("Загрузить изображение")));
	m_menu1->Append( m_menuItem1 );

	wxMenuItem* m_menuItem2 = new wxMenuItem( m_menu1, wxID_ANY, wxString(wxT("Сохранить изображение")));
	m_menu1->Append( m_menuItem2 );
	m_menuItem2->Enable( false );

	m_menu1->AppendSeparator();

	wxMenuItem* m_menuItem3;
	m_menuItem3 = new wxMenuItem( m_menu1, wxID_EXIT, wxString( wxT("Выход") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem3 );

	m_menubar1->Append( m_menu1, wxT("Меню") );

	this->SetMenuBar( m_menubar1 );

	wxInitAllImageHandlers();

	wxBoxSizer* bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_image = new ImageWindow( this );
	bSizer1->Add( m_image, 0, wxALL|wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	m_curves = new CorrectionWindow(this);
	bSizer1->Add( m_curves, 0, wxEXPAND | wxALL, 5 );

	SetSizer( bSizer1 );
	bSizer1->SetSizeHints(this);

	//CreateStatusBar();
    //SetSizeHints(1024, 768);
	//Fit();
	Layout();
	Centre( wxBOTH );
	SetIcon({wxT("wxICON_AAA")});
}