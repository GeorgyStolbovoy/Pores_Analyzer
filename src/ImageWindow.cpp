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
        OnSizeLinear();
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