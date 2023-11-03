#include "CorrectionWindow.h"
#include "ImageWindow.h"
#include "Frame.h"
#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include <boost/gil.hpp>
#include <memory>

namespace gil = boost::gil;

wxBEGIN_EVENT_TABLE(CorrectionWindow, wxWindow)
    EVT_PAINT(CorrectionWindow::OnPaint)
    EVT_SIZE(CorrectionWindow::OnSize)
    EVT_KEY_DOWN(CorrectionWindow::OnKeyDown)
    EVT_KEY_UP(CorrectionWindow::OnKeyUp)
    EVT_ENTER_WINDOW(CorrectionWindow::OnMouseEnter)
    EVT_MOTION(CorrectionWindow::OnMouseMove)
    EVT_LEFT_DOWN(CorrectionWindow::OnMouseLeftDown)
    EVT_LEFT_UP(CorrectionWindow::OnMouseLeftUp)
    EVT_MOUSE_CAPTURE_LOST(CorrectionWindow::OnCaptureLost)
wxEND_EVENT_TABLE()

CorrectionWindow::CorrectionWindow(wxWindow *parent) :
    wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize{300, 300}, wxFULL_REPAINT_ON_RESIZE | wxBORDER_SIMPLE | wxWANTS_CHARS),
    saved_width(GetSize().GetWidth()), saved_height(GetSize().GetHeight()),
    points({
        {{0.0, 0.0}, false, true},
        {{double(saved_width)/2, double(saved_height)/2}, false, false},
        {{double(saved_width), double(saved_height)}, false, true}
    })
{
    SetMinSize(wxSize{300, 300});
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxWHITE);

    for (auto it1 = points.begin(), it2 = ++points.begin(), end = points.end(); it2 != end; ++it1, ++it2)
    {
        const wxPoint2DDouble &p1 = it1->point, &p2 = it2->point;
        curves.insert({
            std::pair{&p1, &p2},
            std::pair{wxPoint2DDouble{(p1.m_x + p2.m_x)/2, (p1.m_y + p2.m_y)/2}, false},
        });
    }
}

void CorrectionWindow::ResetHover()
{
    std::visit([](auto&& hvr)
    {
        using T = std::decay_t<decltype(hvr)>;
        if constexpr (std::is_same_v<T, PointComplex*>)
            hvr->hovered = false;
        else if constexpr (std::is_same_v<T, CurveComplex*>)
            hvr->ctrl_point.second = false;
    }, hover);
}

template <bool do_refresh>
void CorrectionWindow::ProcessHover(int mouse_x, int mouse_y)
{
    double x = ConvertMouseX(mouse_x), y = ConvertMouseY(mouse_y);
    for (auto it = points.begin(), end = points.end(); it != end; ++it)
    {
        if (std::abs(it->point.m_x - x) <= 8 && std::abs(it->point.m_y - y) <= 8) [[unlikely]]
        {
            if (!it->hovered)
            {
                ResetHover();
                const_cast<bool&>(it->hovered) = true;
                hover = const_cast<PointComplex*>(&*it);
                SetCursor(wxCursor{wxCURSOR_HAND});
                if constexpr (do_refresh)
                    Refresh();
            }
            return;
        }
        else if (it->hovered) [[unlikely]]
        {
            const_cast<bool&>(it->hovered) = false;
            hover = std::monostate{};
            if constexpr (do_refresh)
                Refresh();
        }
    }
    for (auto it = curves.begin(), end = curves.end(); it != end; ++it)
    {
        if (std::sqrt(std::pow(it->ctrl_point.first.m_x - x, 2) + std::pow(it->ctrl_point.first.m_y - y, 2)) <= 8) [[unlikely]]
        {
            if (!it->ctrl_point.second)
            {
                ResetHover();
                const_cast<CurveComplex&>(*it).ctrl_point.second = true;
                hover = const_cast<CurveComplex*>(&*it);
                SetCursor(wxCursor{wxCURSOR_HAND});
                if constexpr (do_refresh)
                    Refresh();
            }
            return;
        }
        else if (it->ctrl_point.second)
        {
            const_cast<CurveComplex&>(*it).ctrl_point.second = false;
            hover = std::monostate{};
            if constexpr (do_refresh)
                Refresh();
        }
    }
    SetCursor(wxCursor{wxCURSOR_ARROW});
}

void CorrectionWindow::ProcessAdding(int mouse_x, int mouse_y)
{
    double x = ConvertMouseX(mouse_x), y = ConvertMouseY(mouse_y);
    if (auto it_curve = --curves.upper_bound(x); it_curve != curves.end())
    {
        if (y < (std::max)(it_curve->points.first->m_y, it_curve->points.second->m_y) + 10
            && y > (std::min)(it_curve->points.first->m_y, it_curve->points.second->m_y) - 10)
        {
            double kx = it_curve->points.second->m_x - it_curve->points.first->m_x, x0 = (x - it_curve->points.first->m_x)/kx, xk = x0,
                ky = it_curve->points.second->m_y - it_curve->points.first->m_y, y0 = (y - it_curve->points.first->m_y)/ky,
                a = std::pow(10, it_curve->slope), prev, dif = std::numeric_limits<double>::max();
            uint8_t counter = 0;
            do
            {
	            prev = xk;
                xk -= (a*std::pow(xk, 2*a-1) - a*y0*std::pow(xk, a-1) + xk - x0) / (a*(2*a - 1)*std::pow(xk, 2*a - 2) - a*y0*(a - 1)*std::pow(xk, a - 2) + 1);
                if (std::exchange(dif, std::abs(xk - prev)) <= dif) [[unlikely]]
                    goto failure;
            } while(std::abs(xk*kx - prev*kx) > 0.5);
            if (kx*std::sqrt(std::pow(x0 - xk, 2) + std::pow((x0 - xk)/(a*std::pow(xk, a - 1)), 2)) < 30
                && kx*xk >= 1 && kx*xk <= it_curve->points.second->m_x - it_curve->points.first->m_x - 1)
            {
                point_to_add = wxPoint2DDouble{kx*xk + it_curve->points.first->m_x, ky*std::pow(xk, std::pow(10, it_curve->slope)) + it_curve->points.first->m_y};
                Refresh();
                return;
            }
        }
    }
failure:
    if (point_to_add.has_value())
    {
        point_to_add = std::nullopt;
        Refresh();
    }
}

void CorrectionWindow::UpdateImage()
{
    std::unordered_map<uint8_t, uint8_t> histogram{256};
    int height = GetSize().GetHeight();
    double hist_step = GetSize().GetWidth() / double(256);
    auto curve_it = curves.begin();
    for (uint16_t i = 0; i < 256; ++i)
    {
        if (double x = i*hist_step; x < curve_it->points.first->m_x) [[unlikely]]
        {
            histogram[i] = 0;
            continue;
        }
        else [[likely]]
        {
            if (double x = i*hist_step; x <= curve_it->points.second->m_x)
                histogram[i] = std::round(255*curve_it->CalcY(x)/height);
            else if (++curve_it == curves.end()) [[unlikely]]
            {
                for (uint16_t j = i; j < 256; ++j)
                    histogram[j] = 255;
                break;
            }
            else [[likely]]
                histogram[i] = std::round(255*curve_it->CalcY(x)/height);
        }
    }
    static_cast<Frame*>(GetParent())->RefreshImage(std::move(histogram));
}

void CorrectionWindow::OnPaint(wxPaintEvent& event)
{
    wxBufferedPaintDC dc(this);
    dc.Clear();
    wxBrush brush;
    wxPen pen{wxColour{0x00a000}, 3};
    wxSize size = GetSize();
    int width = size.GetWidth(), height = size.GetHeight();
    std::unique_ptr<wxGraphicsContext> gc{wxGraphicsContext::Create(dc)};
    gc->SetTransform(gc->CreateMatrix(1 - 13/double(width), 0, 0, 13/double(height) - 1, 6.5, height - 6.5));
    
    // Ãטסעמדנאללא
    if (auto view = gil::view(ImageWindow::image); !view.empty())
    {
        if (!path_histogram.has_value()) [[unlikely]]
        {
            path_histogram = gc->CreatePath();
            std::unordered_map<uint8_t, uint32_t> histogram{256};
            for (uint16_t i = 0; i < 256; ++i) histogram[i] = 0;
            uint32_t max_count = 0;
            for (auto iter = view.begin(), end = view.end(); iter != end; ++iter)
                if (uint32_t v = ++histogram[*iter]; v > max_count) [[unlikely]]
                    max_count = v;
            float hist_step = width / float(256);
            for (uint16_t i = 0; i < 256; ++i)
                if (histogram[i] > 0)
                    path_histogram.value().AddRectangle(hist_step*(i - 0.475 + i/float(255)), -6.5, 0.95*hist_step, 0.99*height*histogram[i]/double(max_count));
        }
        brush.SetColour(wxColour{0, 0, 0, 127});
        gc->SetBrush(brush);
        gc->FillPath(path_histogram.value());
    }

    wxGraphicsPath path_points = gc->CreatePath(), path_controllers = gc->CreatePath(), path_selected = gc->CreatePath();
    for (auto it = points.begin(), end = points.end();;)
    {
        wxPoint2DDouble cur_p = it->point;
        if (!it->hovered)
            path_points.AddRectangle(cur_p.m_x - 5, cur_p.m_y - 5, 10, 10);
        else
            path_selected.AddRectangle(cur_p.m_x - 5, cur_p.m_y - 5, 10, 10);
        if (++it != end)
        {
            const wxPoint2DDouble &final_p = it->point;
            auto& curve = const_cast<CurveComplex&>(*curves.find(cur_p.m_x));
            if (!curve.path.has_value())
            {
                curve.path = gc->CreatePath();
                wxGraphicsPath& path = curve.path.value();
                auto y = [bx = cur_p.m_x, by = cur_p.m_y, kx = final_p.m_x - cur_p.m_x, ky = final_p.m_y - cur_p.m_y, slope = curve.slope](int x)
                    {return ky*std::pow((x - bx)/kx, std::pow(10, slope)) + by;};
                while (cur_p.m_x < final_p.m_x)
                {
                    path.MoveToPoint(cur_p);
                    cur_p.m_y = y(cur_p.m_x += 1);
                    path.AddLineToPoint(cur_p);
                }
                path.CloseSubpath();
            }
            if (!curve.ctrl_point.second)
                path_controllers.AddCircle(curve.ctrl_point.first.m_x, curve.ctrl_point.first.m_y, 5);
            else
                path_selected.AddCircle(curve.ctrl_point.first.m_x, curve.ctrl_point.first.m_y, 5);
        }
        else
            break;
    }
    path_points.CloseSubpath();
    path_controllers.CloseSubpath();

    pen.SetColour(wxColour{0x00a000});
    gc->SetPen(pen);
    for (const CurveComplex& c: curves)
        gc->StrokePath(c.path.value());

    pen.SetColour(wxColour{0ul});
    brush.SetColour(wxColour{0x00a000});
    gc->SetPen(pen);
    gc->SetBrush(brush);
    gc->DrawPath(path_points);

    brush.SetColour(wxColour{0x00c0c0});
    gc->SetBrush(brush);
    gc->DrawPath(path_controllers);

    if (HasCapture())
        brush.SetColour(wxColour{0xff0000});
    else
        brush.SetColour(wxColour{0x0000ff});
    gc->SetBrush(brush);
    gc->DrawPath(path_selected);

    if (point_to_add.has_value()) [[unlikely]]
    {
        pen.SetColour(wxColour{0, 0, 0, 127});
        brush.SetColour(wxColour{0, 0, 191, 127});
        gc->SetPen(pen);
        gc->SetBrush(brush);
        gc->DrawEllipse(point_to_add.value().m_x - 5, point_to_add.value().m_y - 5, 10, 10);
    }
}

void CorrectionWindow::OnSize(wxSizeEvent& event)
{
    int width = event.GetSize().GetWidth(), height = event.GetSize().GetHeight();
    double scale_w = width / double(saved_width), scale_h = height / double(saved_height);
    wxGraphicsMatrix matrix = wxGraphicsRenderer::GetDefaultRenderer()->CreateMatrix(scale_w, 0, 0, scale_h, 0, 0);
    if (path_histogram.has_value())
        path_histogram.value().Transform(matrix);
    for (const PointComplex& p: points)
    {
        const_cast<wxPoint2DDouble&>(p.point).m_x *= scale_w;
        const_cast<wxPoint2DDouble&>(p.point).m_y *= scale_h;
    }
    for (const CurveComplex& _: curves)
    {
        CurveComplex& c = const_cast<CurveComplex&>(_);
        if (c.path.has_value())
            c.path.value().Transform(matrix);
        c.ctrl_point.first.m_x *= scale_w;
        c.ctrl_point.first.m_y *= scale_h;
    }
    saved_width = width;
    saved_height = height;
}

void CorrectionWindow::OnKeyDown(wxKeyEvent& event)
{
    if (unsigned k = event.GetRawKeyCode(); !ctrl_pressed && k == 0x11) // Ctrl
    {
        ctrl_pressed = true;
        ResetHover();
        hover = std::monostate{};
        captured = std::monostate{};
        SetCursor(wxCursor{wxCURSOR_CROSS});
        ProcessAdding(event.GetX(), event.GetY());
    }
    else if (!alt_pressed && k == 0x12) // Alt
    {
        alt_pressed = true;
        if (std::holds_alternative<CurveComplex*>(hover)) [[unlikely]]
        {
            ResetHover();
            hover = std::monostate{};
        }
        captured = std::monostate{};
        SetCursor(wxCursor{wxCURSOR_PAINT_BRUSH});
    }
}

void CorrectionWindow::OnKeyUp(wxKeyEvent& event)
{
    if (unsigned k = event.GetRawKeyCode(); k == 0x11)
    {
        ctrl_pressed = false;
        point_to_add = std::nullopt;
        ProcessHover<false>(event.GetX(), event.GetY());
        Refresh();
    }
    else if (k == 0x12) // Alt
    {
        alt_pressed = false;
        if (!std::holds_alternative<PointComplex*>(hover))
            ProcessHover<false>(event.GetX(), event.GetY());
        else
            SetCursor(wxCursor{wxCURSOR_HAND});
        Refresh();
    }
}

void CorrectionWindow::OnMouseMove(wxMouseEvent& event)
{
    std::visit([this, &event](auto&& obj)
    {
        if (!event.ControlDown())
        {
            using T = std::decay_t<decltype(obj)>;
            if constexpr (std::is_same_v<T, PointComplex*>)
            {
                wxSize _ = GetSize();
                int width =  _.GetWidth(), height = _.GetHeight();
                double x = ConvertMouseX(event.GetX()), y = ConvertMouseY(event.GetY());
                if (bool is_first = obj->point.m_x == points.begin()->point.m_x, is_last = obj->point.m_x == (--points.end())->point.m_x;
                    !obj->edge || is_first && obj->point.m_y == 0 || is_last && obj->point.m_y == height)
                {
                    if (int r_bound = (!is_last ? (++points.find(*obj))->point.m_x - 1 : width); x >= r_bound) [[unlikely]]
                        obj->point.m_x = r_bound;
                    else [[likely]] if (int l_bound = (!is_first ? (--points.find(*obj))->point.m_x + 1 : 0); x <= l_bound) [[unlikely]]
                        obj->point.m_x = l_bound;
                    else [[likely]]
                        obj->point.m_x = x;
                }
                if (!obj->edge
                    || obj->point.m_x == points.begin()->point.m_x && obj->point.m_x == 0
                    || obj->point.m_x == (--points.end())->point.m_x && obj->point.m_x == width)
                {
                    if (y >= height) [[unlikely]]
                        obj->point.m_y = height;
                    else [[likely]] if (y <= 0) [[unlikely]]
                        obj->point.m_y = 0;
                    else [[likely]]
                        obj->point.m_y = y;
                }
                auto it = points.find(*obj);
                if (it != points.begin())
                {
                    auto prev = it;
                    auto& prev_curve = const_cast<CurveComplex&>(*curves.find((--prev)->point.m_x));
                    prev_curve.CalcCtrlPoint();
                    prev_curve.path = std::nullopt;
                }
                if (it != --points.end())
                {
                    auto next = it;
                    auto& next_curve = const_cast<CurveComplex&>(*curves.find(it->point.m_x));
                    next_curve.CalcCtrlPoint();
                    next_curve.path = std::nullopt;
                }
                UpdateImage();
                Refresh();
            }
            else if constexpr (std::is_same_v<T, CurveComplex*>)
            {
                double mouse_y = (ConvertMouseY(event.GetY()) - obj->points.first->m_y) / (obj->points.second->m_y - obj->points.first->m_y);
                if (mouse_y >= 0.9330329915) [[unlikely]]
                {
                    if (std::exchange(obj->slope, -1.0f) == -1.0f)
                        return;
                }
                else [[likely]] if (mouse_y <= 0.0009765625) [[unlikely]]
                {
                    if (std::exchange(obj->slope, 1.0f) == 1.0f)
                        return;
                }
                else [[likely]]
                    obj->slope = std::log10(std::log10(mouse_y) / std::log10((obj->ctrl_point.first.m_x - obj->points.first->m_x)/(obj->points.second->m_x - obj->points.first->m_x)));;
                obj->CalcCtrlPoint();
                obj->path = std::nullopt;
                UpdateImage();
                Refresh();
            }
            else if constexpr (std::is_same_v<T, std::monostate>)
            {
                if (!event.AltDown())
                    ProcessHover<true>(event.GetX(), event.GetY());
                else
                {
                    double x = ConvertMouseX(event.GetX()), y = ConvertMouseY(event.GetY());
                    for (auto it = points.begin(), end = points.end(); it != end; ++it)
                    {
                        if (std::abs(it->point.m_x - x) <= 8 && std::abs(it->point.m_y - y) <= 8) [[unlikely]]
                        {
                            if (!it->hovered)
                            {
                                ResetHover();
                                const_cast<bool&>(it->hovered) = true;
                                hover = const_cast<PointComplex*>(&*it);
                                Refresh();
                            }
                            break;
                        }
                        else if (it->hovered) [[unlikely]]
                        {
                            const_cast<bool&>(it->hovered) = false;
                            hover = std::monostate{};
                            Refresh();
                        }
                    }
                }
            }
        }
        else
            ProcessAdding(event.GetX(), event.GetY());
    }, captured);
}

void CorrectionWindow::OnMouseLeftDown(wxMouseEvent& event)
{
    if (!event.ControlDown() && !event.AltDown())
    {
        CaptureMouse();
        if (!std::holds_alternative<std::monostate>(hover))
        {
            captured = hover;
            SetCursor(wxCursor{wxCURSOR_SIZING});
        }
    }
}

void CorrectionWindow::OnMouseLeftUp(wxMouseEvent& event)
{
    if (HasCapture())
    {
        ReleaseMouse();
        ResetHover();
        hover = std::monostate{};
        captured = std::monostate{};
        ProcessHover<false>(event.GetX(), event.GetY());
        Refresh();
    }
    else if (event.ControlDown() && point_to_add.has_value())
    {
        auto& curve = const_cast<CurveComplex&>(*--curves.upper_bound(point_to_add.value().m_x));
        const wxPoint2DDouble* added = &points.insert(PointComplex{wxPoint2DDouble{point_to_add.value().m_x, point_to_add.value().m_y}, false, false}).first->point;
        curves.emplace(CurveComplex::CurvePoints{added, curve.points.second}, &curve);
        curve.ctrl_point.first.m_x = (added->m_x + curve.points.first->m_x)/2;
        curve.ctrl_point.first.m_y = curve.CalcY(curve.ctrl_point.first.m_x, curve.slope, curve.points);
        curve.points.second = added;
        curve.CalcSlope();
        curve.path = std::nullopt;
        point_to_add = std::nullopt;
        Refresh();
    }
    else if (event.AltDown())
    {
        if (PointComplex** point = std::get_if<PointComplex*>(&hover); point) [[unlikely]]
        {
            if (auto point_it = points.find(**point); point_it != points.begin() && point_it != --points.end())
            {
                auto curve_it = curves.find((*point)->point.m_x), next_curve = curve_it;
                CurveComplex& curve = const_cast<CurveComplex&>(*--curve_it);
                next_curve = curve_it;
                curve.points.second = (++next_curve)->points.second;
                curve.ctrl_point.first.m_x = (curve.points.first->m_x + curve.points.second->m_x)/2;
                curve.ctrl_point.first.m_y = (curve.points.first->m_y + curve.points.second->m_y)/2;
                curve.slope = 0.0f;
                curve.path = std::nullopt;
                curves.erase(next_curve);
                points.erase(point_it);
                UpdateImage();
                Refresh();
            }
        }
    }
}