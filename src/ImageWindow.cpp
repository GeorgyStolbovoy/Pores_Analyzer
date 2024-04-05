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
    EVT_MOUSE_CAPTURE_LOST(ImageWindow::OnCaptureLost)
    EVT_TIMER(wxID_ANY, ImageWindow::OnTimer)
wxEND_EVENT_TABLE()

ImageWindow::ImageWindow() : wxWindow(Frame::frame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
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
            double final_ratio = std::min(win_w/double(scaled_width), win_h/double(scaled_height));
            std::ptrdiff_t new_w = nearest_integer<int>(final_ratio*scaled_width), new_h = nearest_integer<int>(final_ratio*scaled_height);
            wxCoord ofs_x = nearest_integer<wxCoord>(win_w/2.0f - new_w/2.0f), ofs_y = nearest_integer<wxCoord>(win_h/2.0f - new_h/2.0f);
            
            wxImage img = marked_image.GetSubImage({xx, yy, nearest_integer<int>(scaled_width), nearest_integer<int>(scaled_height)});
            img.Rescale(new_w, new_h);
            dc.DrawBitmap(wxBitmap{img}, ofs_x, ofs_y);

            if (m_sel_session.has_value())
            {
                wxImage img_sel{w, h, m_sel_session.value().selected_pores, m_sel_session.value().alpha, true};
                img_sel.Resize({nearest_integer<int>(scaled_width), nearest_integer<int>(scaled_height)}, {-xx, -yy});
                img_sel.Rescale(new_w, new_h);
                dc.DrawBitmap(wxBitmap{img_sel}, ofs_x, ofs_y);
            }

            if (m_rec_background.has_value())
            {
                wxImage img_rec{w, h, m_rec_background.value().background, m_rec_background.value().alpha, true};
                img_rec.Resize({nearest_integer<int>(scaled_width), nearest_integer<int>(scaled_height)}, {-xx, -yy});
                img_rec.Rescale(new_w, new_h);
                dc.DrawBitmap(wxBitmap{img_rec}, ofs_x, ofs_y);
            }
        };

        if (!marked_image.IsOk())
        {
	        ptrdiff_t w = image.width(), h = image.height();
            MeasureWindow* measure = Frame::frame->m_measure;
            uint8_t transparency = nearest_integer<uint8_t>(255 * float(measure->m_slider_transparency->GetValue()) / 100);
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
                {
                    if (state == State::RECOVERING)
                    {
                        m_rec_background.emplace(w*h);
                        for (uint32_t del_id : measure->m_deleted_pores)
                        {
	                        uint8_t* color = &measure->m_colors[3*(del_id-1)];
	                        auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().find(del_id), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
                            do
                            {
                                std::ptrdiff_t mem_ofs = it->second.first + it->second.second * w;
                                std::memcpy(buf + 3*mem_ofs, color, 3);
                                alpha[mem_ofs] = 255 - transparency;
                                m_rec_background.value().alpha[mem_ofs] = 0;
                            } while (++it != end && it->first == del_id);
                        }
                    }
                    mem_dc.DrawBitmap(wxBitmap{wxImage{w, h, buf, alpha, true}}, 0, 0);
                }
                else
                {
                    if (state == State::RECOVERING)
                    {
                        m_rec_background.emplace(w*h);
                        for (uint32_t del_id : measure->m_deleted_pores)
                        {
                            auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().find(del_id), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
                            do
                            {
                                m_rec_background.value().alpha[it->second.first + it->second.second * w] = 0;
                            } while (++it != end && it->first == del_id);
                        }
                    }
                    mem_dc.SelectObjectAsSource(wxBitmap{wxImage{w, h, buf, true}});
                }
                marked_image = mem_dc.GetSelectedBitmap().ConvertToImage();
                paint(w, h);
            };

	        if (measure->m_toggle_colorize->GetValue())
	        {
	            if (transparency == 0 && measure->m_deleted_pores.empty() && measure->m_filtered_pores.empty() && image.width()*image.height() == measure->m_pores.size()) [[unlikely]]
	            {
	                auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().begin(), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
	                for (uint32_t i = it->first;; i = it->first)
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
	                }
	            }
	            else [[likely]] if (transparency < 255) 
	            {
	                fill();
                    std::unique_ptr<unsigned char[]> alpha{new unsigned char[w*h]{}};
	                auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().begin(), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
	                for (uint32_t i = it->first;; i = it->first)
	                {
	                    if (!measure->m_deleted_pores.contains(i) && !measure->m_filtered_pores.contains(i))
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
    if (state == State::SCALING && event.LeftIsDown())
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
    switch (state)
    {
    case State::SCALING:
    {
        mouse_last_pos.x = event.GetX();
        mouse_last_pos.y = event.GetY();
        break;
    }
    case State::SELECTING:
    {
        bool alt_down = event.AltDown();
        MeasureWindow* measure = Frame::frame->m_measure;
        if (auto find_it = measure->m_pores.get<MeasureWindow::tag_hashset>().find(
            MeasureWindow::coord_t{nearest_integer<int>(scale_center.x + (event.GetX() - GetSize().GetWidth()/2.0f)/scale_ratio) - 1, nearest_integer<int>(scale_center.y + (event.GetY() - GetSize().GetHeight()/2.0f)/scale_ratio) - 1}
        ), end_it = measure->m_pores.get<MeasureWindow::tag_hashset>().end(); find_it != end_it && !measure->m_deleted_pores.contains(find_it->first) && !measure->m_filtered_pores.contains(find_it->first))
        {
            bool already_selected = measure->m_selected_pores.contains(find_it->first);
            if (!alt_down && !already_selected)
            {
                if (!m_sel_session.has_value())
                    m_sel_session.emplace(this);
                else if (!event.ControlDown())
                {
                    std::memset(m_sel_session.value().alpha, 0, m_sel_session.value().bufsize);
                    measure->m_selected_pores.clear();
                    Frame::frame->m_statistic->pores_statistic_list->deselect_all();
                }
                m_sel_session.value().select(find_it->first);
                Frame::frame->m_statistic->pores_statistic_list->select_item(find_it->first);
            }
            else if (alt_down && already_selected)
            {
                m_sel_session.value().deselect(find_it->first);
                Frame::frame->m_statistic->pores_statistic_list->deselect_item(find_it->first);
                if (measure->m_selected_pores.empty()) [[unlikely]]
                    m_sel_session = std::nullopt;
            }
            else
                break;
        }
        else if (!event.ControlDown() && !alt_down && m_sel_session.has_value())
        {
            m_sel_session = std::nullopt;
            measure->m_selected_pores.clear();
            Frame::frame->m_statistic->pores_statistic_list->deselect_all();
        }
        else
			break;
        Refresh();
        Update();
        break;
    }
    case State::DELETING:
	{
        MeasureWindow* measure = Frame::frame->m_measure;
        if (auto find_it = measure->m_pores.get<MeasureWindow::tag_hashset>().find(MeasureWindow::coord_t{
            	nearest_integer<int>(scale_center.x + (event.GetX() - GetSize().GetWidth()/2.0f)/scale_ratio), nearest_integer<int>(scale_center.y + (event.GetY() - GetSize().GetHeight()/2.0f)/scale_ratio)
        }), end_it = measure->m_pores.get<MeasureWindow::tag_hashset>().end(); find_it != end_it && !measure->m_deleted_pores.contains(find_it->first) && !measure->m_filtered_pores.contains(find_it->first))
        {
            measure->m_deleted_pores.insert(find_it->first);
            marked_image = wxNullImage;
            if (measure->m_selected_pores.contains(find_it->first))
            {
	            m_sel_session.value().deselect(find_it->first);
                Frame::frame->m_statistic->pores_statistic_list->deselect_item(find_it->first);
	            if (measure->m_selected_pores.empty()) [[unlikely]]
	                m_sel_session = std::nullopt;
            }
            Frame::frame->m_statistic->pores_statistic_list->on_pore_deleted(find_it->first);
            Refresh();
            Update();
        }
        break;
    }
    case State::RECOVERING:
    {
        MeasureWindow* measure = Frame::frame->m_measure;
        if (auto find_it = measure->m_pores.get<MeasureWindow::tag_hashset>().find(MeasureWindow::coord_t{
            nearest_integer<int>(scale_center.x + (event.GetX() - GetSize().GetWidth()/2.0f)/scale_ratio), nearest_integer<int>(scale_center.y + (event.GetY() - GetSize().GetHeight()/2.0f)/scale_ratio)
        }), end_it = measure->m_pores.get<MeasureWindow::tag_hashset>().end(); find_it != end_it && measure->m_deleted_pores.contains(find_it->first))
        {
            m_rec_background.value().recover(find_it->first);
            Frame::frame->m_statistic->pores_statistic_list->on_pore_recovered(find_it->first);
            Refresh();
            Update();
        }
	    break;
    }
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
}

void ImageWindow::OnMouseRightUp(wxMouseEvent& event)
{
    if (HasCapture())
    {
        ReleaseMouse();
        if (state == State::SCALING)
        {
            ;
        }
        else if (state == State::SELECTING)
        {
            ;
        }
    }
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

void ImageWindow::OnTimer(wxTimerEvent& event)
{
    SelectionSession& session = m_sel_session.value();
    float color_k = std::sinf(session.rad += float(M_PI/10));
    for (std::size_t i = 0; i < session.bufsize; ++i)
        if (session.alpha[i] > 0) [[unlikely]]
        {
            session.selected_pores[3*i] = 127*(1 + color_k);
            session.selected_pores[3*i+1] = 127*(1 - color_k);
        }
    Refresh();
    Update();
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
    m_sel_session = std::nullopt;
    image.recreate(image_source.dimensions());
    auto src_view = gil::view(image_source), view = gil::view(image);
    for (auto src_it = src_view.begin(), src_end = src_view.end(), dst_it = view.begin(), dst_end = view.end(); src_it != src_end; ++src_it, ++dst_it)
        *dst_it = histogram[*src_it];
    switch (state)
    {
        case State::SCALING: SetCursor(wxCursor{wxCURSOR_MAGNIFIER}); break;
        case State::SELECTING: SetCursor(wxCursor{wxCURSOR_CROSS}); break;
        case State::DELETING: SetCursor(wxCursor{wxCURSOR_BULLSEYE}); break;
        case State::RECOVERING: SetCursor(wxCursor{wxCURSOR_CHAR}); break;
    }
}

void ImageWindow::ApplyHistogram(Histogram_t& hist)
{
    histogram = hist;
    auto src_view = gil::view(image_source), view = gil::view(image);
    for (auto src_it = src_view.begin(), src_end = src_view.end(), dst_it = view.begin(), dst_end = view.end(); src_it != src_end; ++src_it, ++dst_it)
        *dst_it = histogram[*src_it];
    Frame::frame->m_measure->NewMeasure(view);
    marked_image = wxNullImage;
    m_sel_session = std::nullopt;
    Refresh();
    Update();
}

ImageWindow::SelectionSession::SelectionSession(ImageWindow* owner)
	: bufsize(owner->image.width()*owner->image.height()), selected_pores(new unsigned char[bufsize*3]), alpha(new unsigned char[bufsize]{})
{
    SetOwner(owner);
    Start(100);
}

void ImageWindow::SelectionSession::select(uint32_t pore_id)
{
    Frame::frame->m_measure->m_selected_pores.insert(pore_id);
    auto pore_it = Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_multiset>().find(pore_id), pore_end = Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_multiset>().end();
    uint32_t i = pore_it->first;
    do
    {
        alpha[pore_it->second.first + pore_it->second.second * Frame::frame->m_measure->width] = 255;
    } while (++pore_it != pore_end && pore_it->first == i);
}

void ImageWindow::SelectionSession::deselect(uint32_t pore_id)
{
    Frame::frame->m_measure->m_selected_pores.erase(pore_id);
    auto pore_it = Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_multiset>().find(pore_id), pore_end = Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_multiset>().end();
    do
    {
        alpha[pore_it->second.first + pore_it->second.second * Frame::frame->m_measure->width] = 0;
    } while (++pore_it != pore_end && pore_it->first == pore_id);
}

ImageWindow::RecoveringBackground::RecoveringBackground(std::size_t bufsize) : background(new unsigned char[bufsize*3]{}), alpha(new unsigned char[bufsize])
{
    for (std::size_t i = 0; i < 3*bufsize; i += 3)
    {
        background[i] = 10;
        background[i+1] = 10;
        background[i+2] = 10;
    }
    std::memset(alpha, 223, bufsize);
}

void ImageWindow::RecoveringBackground::recover(uint32_t pore_id)
{
    Frame::frame->m_measure->m_deleted_pores.erase(pore_id);
    auto pore_it = Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_multiset>().find(pore_id), pore_end = Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_multiset>().end();
    do
    {
        alpha[pore_it->second.first + pore_it->second.second * Frame::frame->m_measure->width] = 223;
    } while (++pore_it != pore_end && pore_it->first == pore_id);
}