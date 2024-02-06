#include "ImageWindow.h"
#include "Frame.h"
#include "Utils.h"
#include <wx/dcbuffer.h>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/tiff.hpp>
#include <boost/gil/extension/io/bmp.hpp>

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
        auto paint = [this, &dc](std::ptrdiff_t w, std::ptrdiff_t h)
        {
            std::ptrdiff_t win_w = GetSize().GetWidth(), win_h = GetSize().GetHeight(), scaled_height = nearest_integer<int>(win_h/scale_ratio), scaled_width = nearest_integer<int>(win_w/scale_ratio);
            if (scaled_height > h)
                scaled_height = h;
            if (scaled_width > w)
                scaled_width = w;
            int xx, yy;
            if (scale_center.x <= scaled_width/2)
            {
                xx = 0;
                scale_center.x = nearest_integer<int>(scaled_width/2.0f);
            }
            else if (scale_center.x < w - scaled_width/2)
                xx = nearest_integer<int>(scale_center.x - scaled_width/2.0f);
            else
            {
                xx = w - scaled_width;
                scale_center.x = nearest_integer<int>(w - scaled_width/2.0f);
            }
            if (scale_center.y <= scaled_height/2)
            {
                yy = 0;
                scale_center.y = nearest_integer<int>(scaled_height/2.0f);
            }
            else if (scale_center.y < h - scaled_height/2)
                yy = nearest_integer<int>(scale_center.y - scaled_height/2.0f);
            else
            {
                yy = h - scaled_height;
                scale_center.y = nearest_integer<int>(h - scaled_height/2.0f);
            }
            wxImage img = marked_image.GetSubImage({xx, yy, nearest_integer<int>(scaled_width), nearest_integer<int>(scaled_height)});
            double final_ratio = std::min(win_w/double(scaled_width), win_h/double(scaled_height));
            std::ptrdiff_t new_w = nearest_integer<int>(final_ratio*scaled_width), new_h = nearest_integer<int>(final_ratio*scaled_height);
            wxCoord ofs_x = nearest_integer<wxCoord>(win_w/2.0f - new_w/2.0f), ofs_y = nearest_integer<wxCoord>(win_h/2.0f - new_h/2.0f);
            img.Rescale(new_w, new_h);
            dc.DrawBitmap(wxBitmap{img}, ofs_x, ofs_y);
        };

        if (!marked_image.IsOk())
        {
	        ptrdiff_t w = image.width(), h = image.height();
            std::unique_ptr<unsigned char[]> bm_buf{new unsigned char[w*h*3]{}};
            wxMemoryDC mem_dc;
            auto fill = [=, buf = bm_buf.get(), &view, &mem_dc]()
            {
                uint8_t* ptr = buf;
                for (auto it = view.begin(), end = view.end(); it != end; ++it, ptr += 3)
                    memset(ptr, *it, 3);
                mem_dc.SelectObjectAsSource(wxBitmap{wxImage{w, h, buf, true}});
            };
            auto finalize = [=, &mem_dc, &paint, buf = bm_buf.get()](unsigned char* alpha = nullptr)
            {
                if (alpha)
                    mem_dc.DrawBitmap(wxBitmap{wxImage{w, h, buf, alpha, true}}, 0, 0);
                else
                    mem_dc.SelectObjectAsSource(wxBitmap{wxImage{w, h, buf, true}});
                marked_image = mem_dc.GetSelectedBitmap().ConvertToImage();
                paint(w, h);
            };

	        if (MeasureWindow* measure = static_cast<Frame*>(GetParent())->m_measure; measure->m_checkBox_colorize->IsChecked())
	        {
	            if (uint8_t transparency = std::round(255 * float(measure->m_slider_transparency->GetValue()) / 100);
	                transparency == 0 && measure->m_deleted_pores.empty() && image.width()*image.height() == measure->m_pores.size()) [[unlikely]]
	            {
	                auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().begin(), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
	                uint32_t i = it->first;
	                for (;;)
	                {
	                    uint8_t* color = &measure->m_colors[3*(i-1)];
	                    do
	                    {
	                        std::memcpy(bm_buf.get() + 3*(it->second.first + it->second.second * w), color, 3);
	                        if (++it == end) [[unlikely]]
                            {
                                finalize();
	                            return;
                            }
	                    } while (it->first == i);
	                    i = it->first;
	                }
	            }
	            else [[likely]] if (transparency < 255) 
	            {
	                fill();
                    std::unique_ptr<unsigned char[]> alpha{new unsigned char[w*h]{}};
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
	                            std::memcpy(bm_buf.get() + 3*ofs, color, 3);
	                            alpha.get()[ofs] = 255 - transparency;
	                            if (++it == end) [[unlikely]]
                                {
                                    finalize(alpha.get());
	                                return;
                                }
	                        } while (it->first == i);
	                    }
	                    else if ((it = measure->m_pores.get<MeasureWindow::tag_multiset>().lower_bound(i, [](uint32_t lhs, uint32_t rhs) {return lhs <= rhs;}))
	                        == measure->m_pores.get<MeasureWindow::tag_multiset>().end()) [[unlikely]]
                        {
                            finalize(alpha.get());
	                        return;
                        }
	                    i = it->first;
	                }
	            }
	            else
                {
	                fill();
                    finalize();
                }
	        }
	        else
            {
	            fill();
                finalize();
            }
        }
        else
            paint(image.width(), image.height());
    }
}

void ImageWindow::OnSize(wxSizeEvent& event)
{
    int w = event.GetSize().GetWidth(), h = event.GetSize().GetHeight();
    DCbuffer.Create(w, h);
    if (double min_scale = std::min({w / double(image_source.width()), h / double(image_source.height()), 1.0}); min_scale > scale_ratio)
        scale_ratio = min_scale;
}

void ImageWindow::OnMouseMove(wxMouseEvent& event)
{
    if (state == State::SCALING && HasCapture())
    {
        scale_center.x += nearest_integer<int>((mouse_last_pos.x - event.GetX())/scale_ratio);
        scale_center.y += nearest_integer<int>((mouse_last_pos.y - event.GetY())/scale_ratio);
        mouse_last_pos.x = event.GetX();
        mouse_last_pos.y = event.GetY();
        Refresh();
        Update();
    }
}

void ImageWindow::OnMouseLeftDown(wxMouseEvent& event)
{
    CaptureMouse();
    if (state == State::SCALING)
    {
        mouse_last_pos.x = event.GetX();
        mouse_last_pos.y = event.GetY();
    }
    else if (state == State::SELECTING)
    {
        MeasureWindow* measure = static_cast<Frame*>(GetParent())->m_measure;
        if (auto find_it = measure->m_pores.get<MeasureWindow::tag_hashset>().find(
                MeasureWindow::pores_container::value_type::second_type{nearest_integer<int>(scale_center.x + (event.GetX() - GetSize().GetWidth()/2.0f)/scale_ratio), nearest_integer<int>(scale_center.y + (event.GetY() - GetSize().GetHeight()/2.0f)/scale_ratio)}
            ); find_it != measure->m_pores.get<MeasureWindow::tag_hashset>().end() && !measure->m_deleted_pores.contains(find_it->first))
            measure->m_selected_pores.insert(find_it->first);
    }
}

void ImageWindow::OnMouseLeftUp(wxMouseEvent& event)
{
    if (HasCapture())
        ReleaseMouse();
}

void ImageWindow::OnMouseRightDown(wxMouseEvent& event)
{
    CaptureMouse();
    mouse_last_pos.x = event.GetX();
    mouse_last_pos.y = event.GetY();
}

void ImageWindow::OnMouseRightUp(wxMouseEvent& event)
{
    if (HasCapture())
        ReleaseMouse();
}

void ImageWindow::OnMouseWheel(wxMouseEvent& event)
{
    if (state == State::SCALING)
    {
        if (event.GetWheelRotation() > 0)
        {
            if (double dif = 5 - scale_ratio; dif > 0)
            {
                scale_center.x += nearest_integer<int>((event.GetX() - GetSize().GetWidth()/2.0f)/scale_ratio/4);
                scale_center.y += nearest_integer<int>((event.GetY() - GetSize().GetHeight()/2.0f)/scale_ratio/4);
                scale_ratio += dif >= 0.1 ? 0.1 : dif;
            }
            else
            {
                scale_ratio += dif;
                return;
            }
        }
        else
        {
            if (double dif = scale_ratio - std::min({GetSize().GetWidth() / double(image_source.width()), GetSize().GetHeight() / double(image_source.height()), 1.0});
                dif > 0)
            {
                //scale_center.x -= nearest_integer<int>((event.GetX() - GetSize().GetWidth()/2)/scale_ratio/4);
                //scale_center.y -= nearest_integer<int>((event.GetY() - GetSize().GetHeight()/2)/scale_ratio/4);
                scale_ratio -= dif >= 0.1 ? 0.1 : dif;
            }
            else
            {
                scale_ratio -= dif;
                return;
            }
        }
        Refresh();
        Update();
    }
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
    scale_ratio = std::min({GetSize().GetWidth() / double(image_source.width()), GetSize().GetHeight() / double(image_source.height()), 1.0});
    scale_center = wxPoint{nearest_integer<int>(image_source.width()/2), nearest_integer<int>(image_source.height()/2)};
    marked_image = wxNullImage;
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
    marked_image = wxNullImage;
    Refresh();
    Update();
}