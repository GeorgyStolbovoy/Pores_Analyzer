#include "Frame.h"
#include <wx/filedlg.h>
#include <boost/gil/io/read_image.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/tiff.hpp>
#include <boost/gil/extension/io/bmp.hpp>

wxBEGIN_EVENT_TABLE(Frame, wxWindow)
    EVT_MENU(wxID_OPEN, Frame::Load_Image)
    EVT_MENU(wxID_EXIT, Frame::Exit)
wxEND_EVENT_TABLE()

void Frame::Load_Image(wxCommandEvent& event)
{
    wxFileDialog dlg{this, "Выберите изображение", wxEmptyString, wxEmptyString, "Images (png, jpg, tiff)|*.png;*.jpg;*.jpeg;*.tiff", wxFD_OPEN|wxFD_FILE_MUST_EXIST};
    if (dlg.ShowModal() == wxID_OK)
    {
        if (auto&& path = dlg.GetPath(); path.EndsWith(".png"))
            gil::read_and_convert_image(path.c_str().AsChar(), ImageWindow::image, gil::png_tag());
        else if (path.EndsWith(".jpg") || path.EndsWith(".jpeg"))
            gil::read_and_convert_image(path.c_str().AsChar(), ImageWindow::image, gil::jpeg_tag());
        else if (path.EndsWith(".tiff"))
            gil::read_and_convert_image(path.c_str().AsChar(), ImageWindow::image, gil::tiff_tag());
        else if (path.EndsWith(".bmp"))
            gil::read_and_convert_image(path.c_str().AsChar(), ImageWindow::image, gil::bmp_tag());
        m_image->OnSizeLinear();
        m_curves->path_histogram = std::nullopt;
        Refresh();
    }
}

void Frame::RefreshImage(ImageWindow::Histogram_t&& hist)
{
    m_image->histogram = std::move(hist);
    m_image->OnSizeLinear();
    m_image->Refresh();
    m_image->Update();
}