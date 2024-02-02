#include "ImageWindow.h"
#include "Frame.h"
#include <wx/dcbuffer.h>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/tiff.hpp>
#include <boost/gil/extension/io/bmp.hpp>
#include <thread>

wxBEGIN_EVENT_TABLE(ImageWindow, wxWindow)
    EVT_PAINT(ImageWindow::OnPaint)
    EVT_SIZE(ImageWindow::OnSize)
    EVT_MOTION(ImageWindow::OnMouseMove)
    EVT_LEFT_DOWN(ImageWindow::OnMouseLeftDown)
    EVT_LEFT_UP(ImageWindow::OnMouseLeftUp)
    EVT_RIGHT_DOWN(ImageWindow::OnMouseRightDown)
    EVT_RIGHT_UP(ImageWindow::OnMouseRightUp)
    EVT_MOUSEWHEEL(ImageWindow::OnMouseWheel)
    EVT_LEAVE_WINDOW(ImageWindow::OnLeave)
wxEND_EVENT_TABLE()

ImageWindow::ImageWindow(wxWindow *parent, const wxSize& size) : wxWindow(parent, wxID_ANY, wxDefaultPosition, size, wxFULL_REPAINT_ON_RESIZE)
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

    if (auto view = gil::view(image); !view.empty())
    {
        struct painter
        {
            ImageWindow* self;
            Image_t::view_t view;
            ptrdiff_t w, h, new_w, new_h, ofs_x, ofs_y;
            unsigned char *bm_buf, *alpha = nullptr;
            wxBufferedPaintDC& dc;

            painter(ImageWindow* self, const wxSize& size, wxBufferedPaintDC& dc) : 
                self(self), view(gil::view(self->image)), w(self->image.width()), h(self->image.height()), new_w(self->scale_ratio*w), new_h(self->scale_ratio*h),
                ofs_x(size.GetWidth()/2 - new_w/2), ofs_y(size.GetHeight()/2 - new_h/2), bm_buf(static_cast<unsigned char*>(malloc(w*h*3))), dc(dc)
            {
                if (MeasureWindow* measure = static_cast<Frame*>(self->GetParent())->m_measure; measure->m_checkBox_colorize->IsChecked())
                {
                    if (uint8_t transparency = std::round(255 * float(measure->m_slider_transparency->GetValue()) / 100);
                        transparency == 0 && measure->m_deleted_pores.empty() && self->image.width()*self->image.height() == measure->m_pores.size()) [[unlikely]]
                    {
                        auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().begin(), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
                        uint32_t i = it->first;
                        for (;;)
                        {
                            uint8_t* color = &measure->m_colors[3*(i-1)];
                            do
                            {
                                std::memcpy(bm_buf + 3*(it->second.first + it->second.second * w), color, 3);
                                if (++it == end) [[unlikely]]
                                    return;
                            } while (it->first == i);
                            i = it->first;
                        }
                    }
                    else [[likely]] if (transparency < 255) 
                    {
                        fill();
                        wxImage image{w, h, bm_buf, true};
                        image.Rescale(new_w, new_h);
                        dc.DrawBitmap(wxBitmap{image}, ofs_x, ofs_y);
                        alpha = static_cast<unsigned char*>(calloc(w*h, 1));
                        auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().begin(), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
                        uint32_t i = it->first;
                        for (;;)
                        {
                            if (!measure->m_deleted_pores.contains(i))
                            {
                                uint8_t* color = &measure->m_colors[3*(i-1)];
                                do
                                {
                                    std::ptrdiff_t ofs = it->second.first + it->second.second * w;
                                    std::memcpy(bm_buf + 3*ofs, color, 3);
                                    alpha[ofs] = 255 - transparency;
                                    if (++it == end) [[unlikely]]
                                        return;
                                } while (it->first == i);
                            }
                            else if ((it = measure->m_pores.get<MeasureWindow::tag_multiset>().lower_bound(i, [](uint32_t lhs, uint32_t rhs) {return lhs <= rhs;}))
                                == measure->m_pores.get<MeasureWindow::tag_multiset>().end()) [[unlikely]]
                                    return;
                            i = it->first;
                        }
                    }
                    else
                        fill();
                }
                else
                    fill();
            }
            void fill()
            {
                uint8_t* ptr = bm_buf;
                for (auto it = view.begin(), end = view.end(); it != end; ++it, ptr += 3)
                    memset(ptr, *it, 3);
            }
            ~painter()
            {
                wxImage image;
                if (alpha)
                    image.Create(w, h, bm_buf, alpha);
                else
                    image.Create(w, h, bm_buf);
                image.Rescale(new_w, new_h);
                //image.Resize({self->GetSize().GetWidth(), self->GetSize().GetHeight()}, {0, 0});
                dc.DrawBitmap(wxBitmap{image}, ofs_x, ofs_y);
            }
        } __(this, GetSize(), dc);
    }
}

void ImageWindow::OnSize(wxSizeEvent& event)
{
    int w = event.GetSize().GetWidth(), h = event.GetSize().GetHeight();
    DCbuffer.Create(w, h);
    scale_ratio = std::min(w / double(image_source.width()), h / double(image_source.height()));
}

void ImageWindow::OnMouseMove(wxMouseEvent& event)
{
    if (state == State::SCALING && HasCapture())
    {
        scale_by_click = false;
        ;
    }
}

void ImageWindow::OnMouseLeftDown(wxMouseEvent& event)
{
    CaptureMouse();
    scale_by_click = true;
}

void ImageWindow::OnMouseLeftUp(wxMouseEvent& event)
{
    if (HasCapture())
        ReleaseMouse();
    if (scale_by_click)
        ;
}

void ImageWindow::OnMouseRightDown(wxMouseEvent& event)
{
    CaptureMouse();
    scale_by_click = true;
}

void ImageWindow::OnMouseRightUp(wxMouseEvent& event)
{
    if (HasCapture())
        ReleaseMouse();
}

void ImageWindow::OnMouseWheel(wxMouseEvent& event)
{
    if (event.GetWheelRotation() > 0)
        ;
    else
        ;
}

void ImageWindow::OnLeave(wxMouseEvent& event)
{
    ;
}

void ImageWindow::Load(const wxString& path)
{
    if (path.EndsWith(".png"))
        gil::read_and_convert_image(path.c_str().AsChar(), image_source, gil::png_tag());
    else if (path.EndsWith(".jpg") || path.EndsWith(".jpeg"))
        gil::read_and_convert_image(path.c_str().AsChar(), image_source, gil::jpeg_tag());
    else if (path.EndsWith(".tiff"))
        gil::read_and_convert_image(path.c_str().AsChar(), image_source, gil::tiff_tag());
    else if (path.EndsWith(".bmp"))
        gil::read_and_convert_image(path.c_str().AsChar(), image_source, gil::bmp_tag());
    scale_ratio = std::min(GetSize().GetWidth() / double(image_source.width()), GetSize().GetHeight() / double(image_source.height()));
    scale_center = wxPoint2DInt{image_source.width() / 2, image_source.height() / 2};
    image.recreate(image_source.dimensions());
    auto src_view = gil::view(image_source), view = gil::view(image);
    for (auto src_it = src_view.begin(), src_end = src_view.end(), dst_it = view.begin(), dst_end = view.end(); src_it != src_end; ++src_it, ++dst_it)
        *dst_it = histogram[*src_it];
}

void ImageWindow::ApplyHistogram(Histogram_t& hist)
{
    histogram = hist;
    auto src_view = gil::view(image_source), view = gil::view(image);
    for (auto src_it = src_view.begin(), src_end = src_view.end(), dst_it = view.begin(), dst_end = view.end(); src_it != src_end; ++src_it, ++dst_it)
        *dst_it = histogram[*src_it];
    static_cast<Frame*>(GetParent())->m_measure->NewMeasure(view);
    Refresh();
    Update();
}