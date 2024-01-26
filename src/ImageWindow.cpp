#include "ImageWindow.h"
#include "Frame.h"
#include <wx/dcbuffer.h>
#include <thread>

gil::image<gil::gray8_pixel_t> ImageWindow::image, ImageWindow::image_current;

wxBEGIN_EVENT_TABLE(ImageWindow, wxWindow)
    EVT_PAINT(ImageWindow::OnPaint)
    EVT_SIZE(ImageWindow::OnSize)
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

    if (auto view = gil::view(image_current); !view.empty())
    {
        struct painter
        {
            ImageWindow* self;
            decltype(view) view;
            ptrdiff_t w, h, ofs_x, ofs_y;
            unsigned char *bm_buf, *alpha = nullptr;
            wxBufferedPaintDC& dc;

            painter(ImageWindow* self, const wxSize& size, wxBufferedPaintDC& dc) : 
                self(self), view(gil::view(self->image_current)), w(self->image_current.width()), h(self->image_current.height()),
                ofs_x(size.GetWidth()/2 - w/2), ofs_y(size.GetHeight()/2 - h/2), bm_buf(static_cast<unsigned char*>(malloc(w*h*3))), dc(dc)
            {
                if (MeasureWindow* measure = static_cast<Frame*>(self->GetParent())->m_measure; measure->m_checkBox_colorize->IsChecked())
                {
                    if (uint8_t transparency = std::round(255 * float(measure->m_slider_transparency->GetValue()) / 100); transparency == 0) [[unlikely]]
                    {
                        auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().begin(), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
                        uint32_t i = it->first;
                        for (;;)
                        {
                            if (!measure->m_deleted_pores.contains(i))
                            {
                                uint8_t* color = &measure->m_colors[3*i];
                                do
                                {
                                    std::memcpy(bm_buf + 3*(it->second.first + it->second.second * w), color, 3);
                                    if (++it == end) [[unlikely]]
                                        return;
                                } while (it->first == i);
                            }
                            else
                            {
                                do
                                {
                                    uint8_t src_color = *view.at(it->second.first, it->second.second), src_colors[]{src_color, src_color, src_color};
                                    std::memcpy(bm_buf + 3*(it->second.first + it->second.second * w), src_colors, 3);
                                    if (++it == end) [[unlikely]]
                                        return;
                                } while (it->first == i);
                            }
                            i = it->first;
                        }
                    }
                    else [[likely]] if (transparency < 255) 
                    {
                        fill();
                        dc.DrawBitmap(wxBitmap{wxImage{w, h, bm_buf, true}}, ofs_x, ofs_y);
                        alpha = static_cast<unsigned char*>(calloc(w*h, 1));
                        auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().begin(), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
                        uint32_t i = it->first;
                        for (;;)
                        {
                            if (!measure->m_deleted_pores.contains(i))
                            {
                                uint8_t* color = &measure->m_colors[3*i];
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
                if (alpha)
                    dc.DrawBitmap(wxBitmap{wxImage{w, h, bm_buf, alpha}}, ofs_x, ofs_y);
                else
                    dc.DrawBitmap(wxBitmap{wxImage{w, h, bm_buf}}, ofs_x, ofs_y);
            }
        } __(this, GetSize(), dc);
    }
}

void ImageWindow::OnSize(wxSizeEvent& event)
{
    DCbuffer.Create(event.GetSize().GetWidth(), event.GetSize().GetHeight());
    if (!gil::view(image_current).empty())
    {
        OnSizeLinear();
        static_cast<Frame*>(GetParent())->m_measure->NewMeasure();
    }
}

void ImageWindow::OnSizeLinear()
{
    std::ptrdiff_t width = image.width(), height = image.height();
    double k = (std::min)(GetSize().GetWidth() / double(width), GetSize().GetHeight() / double(height));
    std::ptrdiff_t new_width = std::round(k*width), new_height = std::round(k*height);
    
    image_current.recreate(new_width, new_height);
    auto image_view = gil::view(image), view = gil::view(image_current);

    std::vector<std::thread> threads;
    auto process_chunck = [=, &image_view, &view](std::ptrdiff_t a, std::ptrdiff_t b) mutable
    {
        double dy = image_view.height()/double(view.height()), dx = image_view.width()/double(view.width()), src_y = dy*a;
        auto loc_cur = view.xy_at(0, a), loc_img = image_view.xy_at(0, std::floor(src_y));
        auto right = loc_img.cache_location(1, 0), bottom = loc_img.cache_location(0, 1), r_bottom = loc_img.cache_location(1, 1);
        for (std::ptrdiff_t ind_x = 0, loc_x_ofs, ind_y = std::floor(src_y), loc_y_ofs; a < b; ++a, loc_cur += gil::point<std::ptrdiff_t>(-new_width+1, 1), src_y += dy,
            loc_y_ofs = std::floor(src_y - ind_y), ind_y += loc_y_ofs, loc_x_ofs = 0, loc_img += gil::point<std::ptrdiff_t>(-ind_x, loc_y_ofs), ind_x = 0)
        {
            double src_x = 0;
            for (std::ptrdiff_t x = 0; x < new_width-1; ++x, ++loc_cur.x(), src_x += dx,
                loc_x_ofs = std::floor(src_x - ind_x), ind_x += loc_x_ofs, loc_img.x() += loc_x_ofs)
            {
                *loc_cur = histogram[uint8_t(std::round(
                    *loc_img*(ind_x+1 - src_x)*(ind_y+1 - src_y) +
                    loc_img[right]*(src_x - ind_x)*(ind_y + 1 - src_y) +
                    loc_img[bottom]*(ind_x+1 - src_x)*(src_y - ind_y) +
                    loc_img[r_bottom]*(src_x - ind_x)*(src_y - ind_y)
                ))];
            }
            *loc_cur = histogram[uint8_t(std::round(
                *loc_img*(ind_x+1 - src_x)*(ind_y+1 - src_y) +
                *loc_img*(src_x - ind_x)*(ind_y+1 - src_y) +
                loc_img[bottom]*(ind_x+1 - src_x)*(src_y - ind_y) +
                loc_img[bottom]*(src_x - ind_x)*(src_y - ind_y)
            ))];
        }
    };
    float n = 0;
    for (float chunck = new_height / float(std::thread::hardware_concurrency()); std::round(n + chunck) < new_height; n += chunck)
        threads.emplace_back(process_chunck, std::round(n), std::round(n + chunck));
    process_chunck(std::round(n), new_height-1);
    double dx = 1/k, src_x = 0;
    auto it_cur = view.x_at(0, new_height-1), it_img = image_view.x_at(0, std::floor((new_height-1)/k));
    for (auto end_cur = view.row_end(new_height-1) - 1; it_cur != end_cur; ++it_cur, it_img += std::ptrdiff_t(std::floor(-std::floor(src_x) + (src_x += dx))))
        *it_cur = histogram[uint8_t(std::round(*it_img + (*(it_img + 1) - *it_img)*(src_x - std::floor(src_x))))];
    *it_cur = histogram[*it_img];
    for (auto& t: threads)
        if (t.joinable())
            t.join();
}