#pragma once

#include <wx/window.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <memory>
#include <charconv>

static inline const wxString text_placeholder{wxT("N/A")};
static inline constexpr double slider_h = 4.0, y_offset = 36.0, margin = 40.0, label_width = 25.0;

template <class Callback>
concept DoubleSliderCallback = std::is_invocable_r_v<void, Callback, float, float>;

template <DoubleSliderCallback CallbackMin, DoubleSliderCallback CallbackMax = CallbackMin>
struct DoubleSlider : wxWindow
{
	bool active = false;
	float min = 0.0f, max = 0.0f;

	DoubleSlider(wxWindow* parent, CallbackMin&& callback_min, CallbackMax&& callback_max)
		: wxWindow(parent, wxID_ANY), on_min_change(std::forward<CallbackMin>(callback_min)), on_max_change(std::forward<CallbackMax>(callback_max))
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetMinSize({150, 47});
	}
	void set_values(float min_, float max_)
	{
		if (min_ <= max_)
		{
			min = min_;
			max = max_;
			min_pos = 0.0f;
			max_pos = 1.0f;
			active = true;
		}
	}
	std::pair<float, float> get_values()
	{
		return {(max - min)*min_pos + min, (max - min)*max_pos + min};
	}
	void OnPaint(wxPaintEvent& event)
	{
		wxBufferedPaintDC dc(this);
		auto [w, h] = GetSize();

		dc.Clear();
		std::unique_ptr<wxGraphicsContext> gc{wxGraphicsContext::Create(dc)};

		static const wxColour col_idle{0xCCCCCC}, col_hover{0x88FFFF}, col_capture{0xFF8888};
		wxPen pen{wxColour{0ul}, 1};
		gc->SetPen(pen);
		gc->DrawRoundedRectangle(margin, y_offset - slider_h/2, w - margin*2, slider_h, 1);

		min_slider_path = gc->CreatePath();
		double x_min = (w - 2*margin)*min_pos + margin;
		min_slider_path.AddCircle(x_min, y_offset, 10);
		min_slider_path.MoveToPoint(x_min, y_offset - 10);
		min_slider_path.AddLineToPoint(x_min, y_offset - 20);

		max_slider_path = gc->CreatePath();
		double x_max = (w - 2*margin)*max_pos + margin;
		max_slider_path.AddCircle(x_max, y_offset, 10);
		max_slider_path.MoveToPoint(x_max, y_offset - 10);
		max_slider_path.AddLineToPoint(x_max, y_offset - 20);

		wxBrush brush;
		if (active)
		{
			char buf[16];
			*std::to_chars(buf, buf + 16, min).ptr = '\0';
			wxString label{buf};
			auto [text_width, text_height] = fit_text<true>(gc.get(), label, label_width);
			gc->DrawText(label, label_width - text_width, y_offset - text_height/2);

			*std::to_chars(buf, buf + 16, max).ptr = '\0';
			label.assign(buf);
			std::tie(text_width, text_height) = fit_text<true>(gc.get(), label, label_width);
			gc->DrawText(label, w - label_width, y_offset - text_height/2);

			auto draw_min_slider = [this, buf, &label, gc = gc.get(), &pen, &brush, x_min]() mutable
			{
				*std::to_chars(buf, buf + 16, (max - min)*min_pos + min).ptr = '\0';
				label.assign(buf);
				auto [text_width, text_height] = fit_text<true>(gc, label, margin);
				double offset = std::max(text_width, label_width);
				min_slider_path.AddRectangle(x_min - offset, y_offset - 35, offset, 15);
				if (capture == &min_slider_path) [[unlikely]]
					brush.SetColour(col_capture);
				else if (hover == &min_slider_path) [[unlikely]]
					brush.SetColour(col_hover);
				else
					brush.SetColour(col_idle);
				gc->SetBrush(brush);
				pen.SetColour(wxColour{0x008800});
				gc->SetPen(pen);
				gc->DrawPath(min_slider_path);
				gc->DrawText(label, x_min - text_width - (offset - text_width)/2, y_offset - 35 + (15 - text_height)/2);
			};
			auto draw_max_slider = [this, buf, &label, gc = gc.get(), &pen, &brush, x_max]() mutable
			{
				*std::to_chars(buf, buf + 16, (max - min)*max_pos + min).ptr = '\0';
				label.assign(buf);
				auto [text_width, text_height] = fit_text<true>(gc, label, margin);
				double offset = std::max(text_width, label_width);
				max_slider_path.AddRectangle(x_max, y_offset - 35, offset, 15);
				if (capture == &max_slider_path) [[unlikely]]
					brush.SetColour(col_capture);
				else if (hover == &max_slider_path) [[unlikely]]
					brush.SetColour(col_hover);
				else
					brush.SetColour(col_idle);
				gc->SetBrush(brush);
				pen.SetColour(wxColour{0x000088});
				gc->SetPen(pen);
				gc->DrawPath(max_slider_path);
				gc->DrawText(label, x_max + (offset - text_width)/2, y_offset - 35 + (15 - text_height)/2);
			};
			if (foreground == &min_slider_path)
			{
				draw_max_slider();
				draw_min_slider();
			}
			else
			{
				draw_min_slider();
				draw_max_slider();
			}
		}
		else
		{
			auto [ph_width, ph_height] = fit_text<false>(gc.get(), text_placeholder, label_width);
			gc->DrawText(text_placeholder, label_width - ph_width, y_offset - ph_height/2);
			gc->DrawText(text_placeholder, w - label_width, y_offset - ph_height/2);
			double offset = std::max(ph_width, label_width);
			min_slider_path.AddRectangle(x_min - offset, y_offset - 35, offset, 15);
			max_slider_path.AddRectangle(x_max, y_offset - 35, offset, 15);

			auto draw_min_slider = [this, gc = gc.get(), &pen, &brush]()
			{
				pen.SetColour(wxColour{0x008800});
				gc->SetPen(pen);
				if (capture == &min_slider_path) [[unlikely]]
					brush.SetColour(col_capture);
				else if (hover == &min_slider_path) [[unlikely]]
					brush.SetColour(col_hover);
				else
					brush.SetColour(col_idle);
				gc->SetBrush(brush);
				gc->DrawPath(min_slider_path);
			};
			auto draw_max_slider = [this, gc = gc.get(), &pen, &brush]()
			{
				pen.SetColour(wxColour{0x000088});
				gc->SetPen(pen);
				if (capture == &max_slider_path) [[unlikely]]
					brush.SetColour(col_capture);
				else if (hover == &max_slider_path) [[unlikely]]
					brush.SetColour(col_hover);
				else
					brush.SetColour(col_idle);
				gc->SetBrush(brush);
				gc->DrawPath(max_slider_path);
			};
			if (foreground == &min_slider_path)
			{
				draw_max_slider();
				draw_min_slider();
			}
			else
			{
				draw_min_slider();
				draw_max_slider();
			}

			std::tie(ph_width, ph_height) = fit_text<false>(gc.get(), text_placeholder, margin);
			gc->DrawText(text_placeholder, x_min - ph_width - (offset - ph_width)/2, y_offset - 35 + (15 - ph_height)/2);
			gc->DrawText(text_placeholder, x_max + (offset - ph_width)/2, y_offset - 35 + (15 - ph_height)/2);
		}
	}
	void OnMouseMove(wxMouseEvent& event)
	{
		auto [mx, my] = event.GetPosition();
		if (!capture)
		{
			wxGraphicsPath* background;
			if (foreground == &min_slider_path)
				background = &max_slider_path;
			else
				background = &min_slider_path;
			if (foreground->Contains(mx, my))
			{
				if (hover != foreground) [[unlikely]]
				{
					hover = foreground;
					Refresh();
					Update();
				}
			}
			else if (background->Contains(mx, my))
			{
				std::swap(foreground, background);
				hover = foreground;
				Refresh();
				Update();
			}
			else if (hover) [[unlikely]]
			{
				hover = nullptr;
				Refresh();
				Update();
			}
		}
		else
		{
			if (capture == &min_slider_path)
			{
				float new_pos = min_pos + (mx - last_mx)/(GetSize().GetWidth() - 2*margin);
				if (new_pos > 1)
					new_pos = 1;
				else if (new_pos < 0)
					new_pos = 0;
				if (new_pos != min_pos && new_pos <= max_pos)
				{
					min_pos = new_pos;
					Refresh();
					Update();
				}
			}
			else
			{
				float new_pos = max_pos + (mx - last_mx)/(GetSize().GetWidth() - 2*margin);
				if (new_pos > 1)
					new_pos = 1;
				else if (new_pos < 0)
					new_pos = 0;
				if (new_pos != max_pos && new_pos >= min_pos)
				{
					max_pos = new_pos;
					Refresh();
					Update();
				}
			}
			last_mx = mx;
		}
	}
	void OnMouseLeftDown(wxMouseEvent& event)
	{
		CaptureMouse();
		capture = hover;
		hover = nullptr;
		last_mx = event.GetX();
		Refresh();
		Update();
	}
	void OnMouseLeftUp(wxMouseEvent& event)
	{
		if (HasCapture())
		{
 			if (wxGraphicsPath* slider = std::exchange(capture, nullptr); slider)
			{
				if (auto [min_value, max_value] = get_values(); slider == &min_slider_path)
					on_min_change(min_value, max_value);
				else
					on_max_change(min_value, max_value);
				Refresh();
				Update();
			}
			ReleaseMouse();
		}
	}
	void OnLeave(wxMouseEvent& event)
	{
		if (std::exchange(hover, nullptr)) [[unlikely]]
		{
			Refresh();
			Update();
		}
	}
	void OnCaptureLost(wxMouseCaptureLostEvent& event) {capture = nullptr;}

private:
	float min_pos = 0.0f, max_pos = 1.0f;
	wxGraphicsPath min_slider_path, max_slider_path;
	wxGraphicsPath *hover = nullptr, *capture = nullptr, *foreground = &min_slider_path;
	int last_mx;
	CallbackMin on_min_change;
	CallbackMax on_max_change;

	template <bool format_number>
	std::pair<double, double> fit_text(wxGraphicsContext* gc, std::conditional_t<format_number, wxString, const wxString>& text, const double max_width)
	{
		int initial_size = 10;
		wxFont font{initial_size, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_SEMIBOLD};
		gc->SetFont(font, {0ul});
		double text_width, text_height;
		gc->GetTextExtent(text, &text_width, &text_height);
		if constexpr (format_number)
			while (text_width > max_width)
			{
				font.SetPointSize(--initial_size);
				gc->SetFont(font, {0ul});
				gc->GetTextExtent(text, &text_width, &text_height);
				if (text_width > max_width)
				{
					if (initial_size <= 6) [[unlikely]]
					{
						for (int pos = text.Find('.'); pos != wxNOT_FOUND; pos = text.Find('.'))
						{
							text.RemoveLast();
							if (pos == text.size()-1) [[unlikely]]
							{
								text.RemoveLast();
								break;
							}
							gc->GetTextExtent(text, &text_width, &text_height);
							if (text_width <= max_width) [[unlikely]]
								break;
						}
						break;
					}
				}
				else
					break;
			}
		else
			while (text_width > max_width)
			{
				font.SetPointSize(--initial_size);
				gc->SetFont(font, {0ul});
				gc->GetTextExtent(text_placeholder, &text_width, &text_height);
			}
		return {text_width, text_height};
	}

	wxDECLARE_EVENT_TABLE();
};

#define typename DoubleSliderCallback
wxBEGIN_EVENT_TABLE_TEMPLATE2(DoubleSlider, wxWindow, CallableMin, CallableMax)
	EVT_PAINT(DoubleSlider::OnPaint)
	EVT_MOTION(DoubleSlider::OnMouseMove)
	EVT_LEFT_DOWN(DoubleSlider::OnMouseLeftDown)
	EVT_LEFT_UP(DoubleSlider::OnMouseLeftUp)
	EVT_LEAVE_WINDOW(DoubleSlider::OnLeave)
	EVT_MOUSE_CAPTURE_LOST(DoubleSlider::OnCaptureLost)
wxEND_EVENT_TABLE()
#undef typename