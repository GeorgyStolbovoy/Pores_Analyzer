#include "Frame.h"
#include <wx/filedlg.h>
#include <boost/gil/io/read_image.hpp>

wxBEGIN_EVENT_TABLE(Frame, wxWindow)
    EVT_MENU(wxID_OPEN, Frame::OnLoad)
    EVT_MENU(wxID_EXIT, Frame::Exit<wxCommandEvent>)
    EVT_TOOL(wxID_OPEN, Frame::OnLoad)
    EVT_TOOL(wxID_FIND, Frame::OnToolbarScale)
    EVT_TOOL(wxID_SELECTALL, Frame::OnToolbarSelect)
    EVT_TOOL(wxID_DELETE, Frame::OnToolbarDelete)
    EVT_TOOL(wxID_REVERT, Frame::OnToolbarRecovery)
    EVT_CLOSE(Frame::Exit<wxCloseEvent>)
wxEND_EVENT_TABLE()

void Frame::OnLoad(wxCommandEvent& event)
{
    wxFileDialog dlg{this, wxT("Выберите изображение"), wxEmptyString, wxEmptyString, wxT("Images (png, jpg, tiff)|*.png;*.jpg;*.jpeg;*.tiff"), wxFD_OPEN|wxFD_FILE_MUST_EXIST};
    if (dlg.ShowModal() == wxID_OK)
    {
        m_image->Load(dlg.GetPath());
        auto view = gil::view(m_image->image);
        m_measure->NewMeasure(view);
        if (uint8_t side = m_statistic->settings_window->m_choice_side->GetSelection(); side != 0)
			m_statistic->settings_window->metric_coef = m_statistic->settings_window->m_spinctrl_coef->GetValue() / (side == 1 ? view.width() : view.height());
        m_curves->Enable(true);
        m_curves->path_histogram = std::nullopt;
        Refresh();
        Update();
    }
}

#define SELECT_TOOL(tool, new_cursor) \
	if (!gil::view(m_image->image).empty()) \
	{ \
	    m_image->SetCursor(wxCursor{wxCURSOR_##new_cursor}); \
	    if (std::exchange(m_image->state, ImageWindow::State::tool) == ImageWindow::State::RECOVERING) \
	    { \
			m_image->marked_image = wxNullImage; \
			m_image->m_rec_background = std::nullopt; \
	        m_image->Refresh(); \
	        m_image->Update(); \
	    } \
	} \
	else \
		m_image->state = ImageWindow::State::tool;

void Frame::OnToolbarScale(wxCommandEvent& event)
{
    SELECT_TOOL(SCALING, MAGNIFIER)
}

void Frame::OnToolbarSelect(wxCommandEvent& event)
{
    SELECT_TOOL(SELECTING, HAND)
}

void Frame::OnToolbarDelete(wxCommandEvent& event)
{
    SELECT_TOOL(DELETING, BULLSEYE)
}

void Frame::OnToolbarRecovery(wxCommandEvent& event)
{
    m_image->state = ImageWindow::State::RECOVERING;
    if (!gil::view(m_image->image).empty())
    {
        m_image->marked_image = wxNullImage;
        m_image->SetCursor(wxCursor{wxCURSOR_CROSS});
        m_image->Refresh();
        m_image->Update();
    }
}