#include "ImageWindow.h"
#include <wx/dcbuffer.h>

wxImage ImageWindow::m_img_original, ImageWindow::m_img_edited;
ImageWindow::EStatus ImageWindow::status = ImageWindow::EStatus::COLOR;

wxBEGIN_EVENT_TABLE(ImageWindow, wxWindow)
    EVT_PAINT(ImageWindow::OnPaint)
wxEND_EVENT_TABLE()

void ImageWindow::ToGrayscale()
{
    m_img_original = m_img_original.ConvertToGreyscale();
    status = EStatus::GREY;
}

ImageWindow::ImageWindow(wxWindow *parent) : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
{
    SetMinSize(wxSize{800, 600});
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxBLACK);

    m_img_original = wxImage("test.png");
    ToGrayscale();
    m_img_edited = m_img_original.Copy();
}

void ImageWindow::OnPaint(wxPaintEvent& event)
{
    wxBufferedPaintDC dc(this);
    dc.Clear();
    wxSize size = GetSize();
    float k = (std::min)(size.GetWidth() / float(m_img_edited.GetWidth()), size.GetHeight() / float(m_img_edited.GetHeight()));
    wxImage img = m_img_edited.Scale(m_img_edited.GetWidth()*k, m_img_edited.GetHeight()*k);
    dc.DrawBitmap(img, size.GetWidth()/2 - img.GetWidth()/2, size.GetHeight()/2 - img.GetHeight()/2);
}