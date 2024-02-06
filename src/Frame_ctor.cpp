#include "Frame.h"
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <../art/filesave.xpm>
#include <../art/fileopen.xpm>
#include <../art/find.xpm>
#include <../art/plus.xpm>
#include <../art/delete.xpm>
#include <../art/redo.xpm>

Frame::Frame() : wxFrame(nullptr, wxID_ANY, "Pores Analyzer")
{
	wxSize frame_size{1000, 1200}, monitor_size = wxGetDisplaySize();
	if (float k = (std::min)(frame_size.x / float(monitor_size.x), frame_size.y / float(monitor_size.y)); k > 0.9) [[unlikely]]
	{
		frame_size.x *= 0.9/k;
		frame_size.y *= 0.9/k;
	}

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

	SetMenuBar( m_menubar1 );

	m_toolBar = CreateToolBar( wxTB_FLAT|wxTB_HORIZONTAL|wxTB_TEXT, wxID_ANY );
	m_toolBar->SetBackgroundColour( wxColour( 128, 128, 128 ) );
	wxToolBarToolBase* m_tool_save = m_toolBar->AddTool( wxID_SAVE, wxT("Сохранение"), wxBitmap{filesave_xpm}, wxNullBitmap, wxITEM_NORMAL);

	wxToolBarToolBase* m_tool_load = m_toolBar->AddTool( wxID_OPEN, wxT("Загрузка"), wxBitmap{fileopen_xpm}, wxNullBitmap, wxITEM_NORMAL);

	m_toolBar->AddSeparator();

	wxToolBarToolBase* m_tool_scale = m_toolBar->AddTool( wxID_FIND, wxT("Масштаб"), wxBitmap{find_xpm}, wxNullBitmap, wxITEM_RADIO);

	wxToolBarToolBase* m_tool_select = m_toolBar->AddTool( wxID_SELECTALL, wxT("Выделение пор"), wxBitmap{plus_xpm}, wxNullBitmap, wxITEM_RADIO);

	wxToolBarToolBase* m_tool_background = m_toolBar->AddTool(wxID_DELETE, wxT("Удаление пор"), wxBitmap{delete_xpm}, wxNullBitmap, wxITEM_RADIO);

	wxToolBarToolBase* m_tool_recovery = m_toolBar->AddTool( wxID_REVERT, wxT("Восстан-е пор"), wxBitmap{redo_xpm}, wxNullBitmap, wxITEM_RADIO);

	m_toolBar->AddSeparator();
	m_toolBar->Realize();
	SetToolBar(m_toolBar);

	wxBoxSizer* bSizer_all = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer_imageControl = new wxBoxSizer( wxVERTICAL );

	m_image = new ImageWindow(this, wxSize{frame_size.x, int(frame_size.y*8/11.0f)});
	bSizer_imageControl->Add( m_image, 0, wxALL|wxEXPAND, 5 );

	wxStaticLine* m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer_imageControl->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	m_curves = new CorrectionWindow(this, wxSize{frame_size.x, int(frame_size.y*3/11.0f)});
	m_curves->Enable(false);
	bSizer_imageControl->Add( m_curves, 0, wxEXPAND | wxALL, 5 );
	
	bSizer_all->Add( bSizer_imageControl, 1, wxEXPAND, 5 );

	m_measure = new MeasureWindow(this);

	bSizer_all->Add(m_measure, 0, wxALL|wxEXPAND, 5);

	SetSizer(bSizer_all);
	bSizer_all->SetSizeHints(this);

	CreateStatusBar();
	Layout();
	Centre( wxBOTH );
	SetIcon({wxT("wxICON_AAA")});
}