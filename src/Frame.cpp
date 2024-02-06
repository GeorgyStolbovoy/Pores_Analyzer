#include "Frame.h"
#include <wx/filedlg.h>
#include <boost/gil/io/read_image.hpp>

wxBEGIN_EVENT_TABLE(Frame, wxWindow)
    EVT_MENU(wxID_OPEN, Frame::OnLoad)
    EVT_MENU(wxID_EXIT, Frame::Exit<wxCommandEvent>)
    EVT_TOOL(wxID_SAVE, Frame::OnSave)
    EVT_TOOL(wxID_OPEN, Frame::OnLoad)
    EVT_TOOL(wxID_FIND, Frame::OnToolbarScale)
    EVT_TOOL(wxID_SELECTALL, Frame::OnToolbarSelect)
    EVT_TOOL(wxID_DELETE, Frame::OnToolbarDelete)
    EVT_TOOL(wxID_REVERT, Frame::OnToolbarRecovery)
    EVT_CLOSE(Frame::Exit<wxCloseEvent>)
wxEND_EVENT_TABLE()

void Frame::OnLoad(wxCommandEvent& event)
{
    wxFileDialog dlg{this, "Выберите изображение", wxEmptyString, wxEmptyString, "Images (png, jpg, tiff)|*.png;*.jpg;*.jpeg;*.tiff", wxFD_OPEN|wxFD_FILE_MUST_EXIST};
    if (dlg.ShowModal() == wxID_OK)
    {
        m_image->Load(dlg.GetPath());
        if (m_toolBar->FindById(wxID_FIND)->IsToggled())
            m_image->SetCursor(wxCursor{wxCURSOR_MAGNIFIER});
        else if (m_toolBar->FindById(wxID_SELECTALL)->IsToggled())
            m_image->SetCursor(wxCursor{wxCURSOR_CROSS});
        m_measure->NewMeasure(gil::view(m_image->image));
        m_curves->Enable(true);
        m_curves->path_histogram = std::nullopt;
        Refresh();
        Update();
    }
}

void Frame::OnSave(wxCommandEvent& event)
{
    ;
}

void Frame::OnToolbarScale(wxCommandEvent& event)
{
    m_image->state = ImageWindow::State::SCALING;
    if (!gil::view(m_image->image).empty())
        m_image->SetCursor(wxCursor{wxCURSOR_MAGNIFIER});
}

void Frame::OnToolbarSelect(wxCommandEvent& event)
{
    m_image->state = ImageWindow::State::SELECTING;
    if (!gil::view(m_image->image).empty())
        m_image->SetCursor(wxCursor{wxCURSOR_CROSS});
}

void Frame::OnToolbarDelete(wxCommandEvent& event)
{
    m_image->state = ImageWindow::State::DELETING;
}

void Frame::OnToolbarRecovery(wxCommandEvent& event)
{
    m_image->state = ImageWindow::State::RECOVERING;
}