#include "ImageWindow.h"
#include <wx/dcbuffer.h>
#include <wx/rawbmp.h>

gil::image<gil::gray8_pixel_t> ImageWindow::image, ImageWindow::image_current;

wxBEGIN_EVENT_TABLE(ImageWindow, wxWindow)
    EVT_PAINT(ImageWindow::OnPaint)
    EVT_SIZE(ImageWindow::OnSize)
wxEND_EVENT_TABLE()

void ImageWindow::ApplyHistogram()
{
    gil::for_each_pixel(gil::view(ImageWindow::image_current), [this](gil::gray8_pixel_t& pix){pix = histogram[pix];});
}

ImageWindow::ImageWindow(wxWindow *parent) : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize{800, 600}, wxFULL_REPAINT_ON_RESIZE)
{
    SetMinSize(wxSize{800, 600});
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxBLACK);

    for (uint16_t i = 0; i < 256; ++i)
        histogram[i] = i;
}

void ImageWindow::OnPaint(wxPaintEvent& event)
{
    wxBufferedPaintDC dc(this, DCbuffer);
    dc.Clear();

    if (auto view = gil::view(image_current); !view.empty())
    {
        wxSize size = GetSize();
        ptrdiff_t w = image_current.width(), h = image_current.height(), ofs_x = size.GetWidth()/2 - w/2, ofs_y = size.GetHeight()/2 - h/2;
        unsigned char* bm_buf = new unsigned char[w*h*3], *ptr = bm_buf;
        for (auto it = view.begin(), end = view.end(); it != end; ++it, ptr += 3)
            memset(ptr, *it, 3);
        dc.DrawBitmap(wxBitmap{wxImage{w, h, bm_buf}}, ofs_x, ofs_y);
    }
}

void ImageWindow::OnSize(wxSizeEvent& event)
{
    DCbuffer.Create(event.GetSize().GetWidth(), event.GetSize().GetHeight());
    if (!gil::view(image_current).empty())
        OnSize();
}

void ImageWindow::OnSize()
{
    double width = image.width(), height = image.height(), k = (std::min)(GetSize().GetWidth() / width, GetSize().GetHeight() / height);
    image_current.recreate(k*width, k*height);
    gil::scale_lanczos(gil::view(image), gil::view(image_current), 2);
    ApplyHistogram();
}