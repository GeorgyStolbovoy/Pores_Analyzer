#include "DoubleSlider.h"
#include <wx/dcbuffer.h>
#include <memory>
#include <charconv>

wxBEGIN_EVENT_TABLE(DoubleSlider, wxWindow)
	EVT_PAINT(DoubleSlider::OnPaint)
wxEND_EVENT_TABLE()

const wxString DoubleSlider::text_placeholder{wxT("N/A")};

DoubleSlider::DoubleSlider(wxWindow* parent) : wxWindow(parent, wxID_ANY)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetMinSize({150, 47});
}

void DoubleSlider::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);
	auto [w, h] = GetSize();
	constexpr double slider_h = 4.0, y_offset = 36.0, margin = 40.0, label_width = 25.0;

	dc.Clear();
	std::unique_ptr<wxGraphicsContext> gc{wxGraphicsContext::Create(dc)};

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

	wxBrush brush{wxColour{0xCCCCCC}};
	gc->SetBrush(brush);
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
		gc->DrawText(label, w - margin, y_offset - text_height/2);

		*std::to_chars(buf, buf + 16, (max - min)*min_pos + min).ptr = '\0';
		label.assign(buf);
		std::tie(text_width, text_height) = fit_text<true>(gc.get(), label, margin);
		double offset = std::max(text_width, label_width);
		min_slider_path.AddRectangle(x_min - offset, y_offset - 35, offset, 15);
		pen.SetColour(wxColour{0x008800});
		gc->SetPen(pen);
		gc->DrawPath(min_slider_path);
		gc->DrawText(label, x_min - text_width - (offset - text_width)/2, y_offset - 35 - text_height/2);

		*std::to_chars(buf, buf + 16, (max - min)*max_pos + min).ptr = '\0';
		label.assign(buf);
		std::tie(text_width, text_height) = fit_text<true>(gc.get(), label, margin);
		offset = std::max(text_width, label_width);
		max_slider_path.AddRectangle(x_max, y_offset - 35, offset, 15);
		pen.SetColour(wxColour{0x000088});
		gc->SetPen(pen);
		gc->DrawPath(max_slider_path);
		gc->DrawText(label, x_max + (offset - text_width)/2, y_offset - 35 - text_height/2);
	}
	else
	{
		auto [ph_width, ph_height] = fit_text<false>(gc.get(), text_placeholder, label_width);
		gc->DrawText(text_placeholder, label_width - ph_width, y_offset - ph_height/2);
		gc->DrawText(text_placeholder, w - label_width, y_offset - ph_height/2);
		std::tie(ph_width, ph_height) = fit_text<false>(gc.get(), text_placeholder, margin);
		double offset = std::max(ph_width, label_width);
		min_slider_path.AddRectangle(x_min - offset, y_offset - 35, offset, 15);
		max_slider_path.AddRectangle(x_max, y_offset - 35, offset, 15);
		pen.SetColour(wxColour{0x008800});
		gc->SetPen(pen);
		gc->DrawPath(min_slider_path);
		pen.SetColour(wxColour{0x000088});
		gc->SetPen(pen);
		gc->DrawPath(max_slider_path);
		gc->DrawText(text_placeholder, x_min - ph_width - (offset - ph_width)/2, y_offset - 35 + (15 - ph_height)/2);
		gc->DrawText(text_placeholder, x_max + (offset - ph_width)/2, y_offset - 35 + (15 - ph_height)/2);
	}
}

template <bool format_number>
std::pair<double, double> DoubleSlider::fit_text(wxGraphicsContext* gc, std::conditional_t<format_number, wxString, const wxString>& text, const double max_width)
{
	int initial_size = 10;
	wxFont font{initial_size, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_SEMIBOLD};
	gc->SetFont(font, {0ul});
	double text_width, text_height;
	gc->GetTextExtent(text, &text_width, &text_height);
	if constexpr (format_number)
		while (text_width > max_width)
		{
			if (int pos = text.Find('.'); pos != wxNOT_FOUND)
			{
				text.RemoveLast();
				if (pos == text.size()-1) [[unlikely]]
					text.RemoveLast();
			}
			else
			{
				do
				{
					font.SetPointSize(--initial_size);
					gc->SetFont(font, {0ul});
					gc->GetTextExtent(text, &text_width, &text_height);
				} while (text_height > max_width);
				break;
			}
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