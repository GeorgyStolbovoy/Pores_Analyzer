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
        //auto start = std::chrono::high_resolution_clock::now();
        wxSize size = GetSize();
        ptrdiff_t w = image_current.width(), h = image_current.height(), ofs_x = size.GetWidth()/2 - w/2, ofs_y = size.GetHeight()/2 - h/2;

        /*unsigned char* bm_buf = new unsigned char[w*h*3];
        std::vector<std::thread> threads;
        auto process_chunck = [=, &view](std::ptrdiff_t a, std::ptrdiff_t b)
        {
            unsigned char *ptr = bm_buf + a*w*3;
            for (auto it = view.xy_at(0, a); a < b; ++a, it += gil::point<std::ptrdiff_t>(-w, 1))
                for (std::ptrdiff_t x = 0; x < w; ++x, ++it.x(), ptr += 3)
                    memset(ptr, *it, 3);
        };
        float n = 0;
        for (float chunck = h / float(std::thread::hardware_concurrency()); std::round(n + chunck) < h; n += chunck)
            threads.emplace_back(process_chunck, std::round(n), std::round(n + chunck));
        process_chunck(std::round(n), h);
        for (auto& t: threads) t.join();*/

        unsigned char* bm_buf = new unsigned char[w*h*3], *ptr = bm_buf;
        for (auto it = view.begin(), end = view.end(); it != end; ++it, ptr += 3)
            memset(ptr, *it, 3);

        dc.DrawBitmap(wxBitmap{wxImage{w, h, bm_buf}}, ofs_x, ofs_y);
        //static_cast<Frame*>(GetParent())->SetStatusText(wxString::Format("ms: %i", (int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count()));
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

#if 0
void ImageWindow::OnSizeBicubic()
{
    double width = image.width(), height = image.height(), k = (std::min)(GetSize().GetWidth() / width, GetSize().GetHeight() / height);
    std::ptrdiff_t new_width = std::round(k*width), new_height = std::round(k*height);
    image_current.recreate(new_width, new_height);

    /*auto cubic = [](uint8_t _0, uint8_t _1, uint8_t _2, uint8_t _3, double x) -> uint8_t
    {
        //double result = 2*_1 + (-2*_0 - 3*_1 + 6*_2 - _3)/3*x + (_0 - 2*_1 + _2)*std::pow(x, 2) + (-_0 + 3*_1 - 3*_2 + _3)/3*std::pow(x, 3);
        double result = _1 + (-0.5*_0 + 0.5*_2)*x + (_0 - 5/2*_1 + 2*_2 - 0.5*_3)*std::pow(x, 2) + (-0.5*_0 + 3/2*_1 - 3/2*_2 + 0.5*_3)/3*std::pow(x, 3);
        return std::round(result);
    };*/
    uint8_t values[4][4];
    auto image_view = gil::view(image);
    auto loc_cur = gil::view(image_current).xy_at(0, 0);

    for (std::ptrdiff_t y = 0; y < new_height; ++y, loc_cur += gil::point<std::ptrdiff_t>(-new_width, 1))
    {
        for (std::ptrdiff_t x = 0; x < new_width; ++x, ++loc_cur.x())
        {
            double src_x = x / k, src_y = y / k;

            std::ptrdiff_t ind_x = std::floor(src_x), ind_y = std::floor(src_y);
            double _x = src_x - ind_x, _y = src_y - ind_y;
            for (int8_t i = 0; i < 4; ++i)
            {
                std::ptrdiff_t ind_y_cur = ind_y - 1 + i;
                if (ind_y_cur < 0) [[unlikely]]
                    ind_y_cur = 0;
                else [[likely]] if (ind_y_cur >= height) [[unlikely]]
                    ind_y_cur = height-1;
                for (uint8_t j = 0; j < 4; ++j)
                {
                    std::ptrdiff_t ind_x_cur = ind_x - 1 + j;
                    if (ind_x_cur < 0) [[unlikely]]
                        ind_x_cur = 0;
                    else [[likely]] if (ind_x_cur >= width) [[unlikely]]
                        ind_x_cur = width-1;
                    values[i][j] = *image_view.at(ind_x_cur, ind_y_cur);
                }
            }

            double
                b1 = 0.25*(_x-1)*(_x-2)*(_x+1)*(_y-1)*(_y-2)*(_y+1),
                b2 = -0.25*_x*(_x+1)*(_x-2)*(_y-1)*(_y-2)*(_y+1),
                b3 = -0.25*_y*(_x-1)*(_x-2)*(_x+1)*(_y-2)*(_y+1),
                b4 = 0.25*_x*_y*(_x-2)*(_x+1)*(_y-2)*(_y+1),
                b5 = -1/12*_x*(_x-1)*(_x-2)*(_y-1)*(_y-2)*(_y+1),
                b6 = -1/12*_y*(_x-1)*(_x-2)*(_x+1)*(_y-1)*(_y-2),
                b7 = 1/12*_x*_y*(_x-1)*(_x-2)*(_y-2)*(_y+1),
                b8 = 1/12*_x*_y*(_x-2)*(_x+1)*(_y-1)*(_y-2),
                b9 = 1/12*_x*(_x-1)*(_x+1)*(_y-1)*(_y-2)*(_y+1),
                b10 = 1/12*_y*(_x-1)*(_x-2)*(_x+1)*(_y-1)*(_y+1),
                b11 = 1/36*_x*_y*(_x-1)*(_x-2)*(_y-1)*(_y-2),
                b12 = -1/12*_x*_y*(_x-1)*(_x+1)*(_y-2)*(_y+1),
                b13 = -1/12*_x*_y*(_x-2)*(_x+1)*(_y-1)*(_y+1),
                b14 = -1/36*_x*_y*(_x-1)*(_x+1)*(_y-1)*(_y-2),
                b15 = -1/36*_x*_y*(_x-1)*(_x-2)*(_y-1)*(_y+1),
                b16 = 1/36*_x*_y*(_x-1)*(_x+1)*(_y-1)*(_y+1),

                result = b1*values[1][1] + b2*values[1][2] + b3*values[2][1] + b4*values[2][2] +
                         b5*values[1][0] + b6*values[0][1] + b7*values[2][0] + b8*values[0][2] +
                         b9*values[1][3] + b10*values[3][1] + b11*values[0][0] + b12*values[2][3] +
                         b13*values[3][2] + b14*values[0][3] + b15*values[3][0] + b16*values[3][3];

            *loc_cur = histogram[
                uint8_t(result)
                /*cubic(
                    cubic(values[0][0], values[0][1], values[0][2], values[0][3], _x),
                    cubic(values[1][0], values[1][1], values[1][2], values[1][3], _x),
                    cubic(values[2][0], values[2][1], values[2][2], values[2][3], _x),
                    cubic(values[3][0], values[3][1], values[3][2], values[3][3], _x),
                    _y
                )*/
            ];
        }
    }
}
#endif