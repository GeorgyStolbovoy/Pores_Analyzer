#include "DoubleSlider.h"
#include <wx/dcbuffer.h>
#include <memory>

wxBEGIN_EVENT_TABLE(DoubleSlider, wxWindow)
	EVT_PAINT(DoubleSlider::OnPaint)
wxEND_EVENT_TABLE()

DoubleSlider::DoubleSlider(wxWindow* parent) : wxWindow(parent, wxID_ANY)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetMinSize({150, 47});
}

void DoubleSlider::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);
	auto [w, h] = GetSize();
	constexpr double slider_h = 4, y_offset = 36;
	wxString text_placeholder{wxT("N/A")};
	double text_width, text_height;

	dc.Clear();
	std::unique_ptr<wxGraphicsContext> gc{wxGraphicsContext::Create(dc)};
	wxFont font{10, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_SEMIBOLD};
	gc->SetFont(font, {0ul});
	if (!active)
	{
		gc->GetTextExtent(text_placeholder, &text_width, &text_height);
		gc->DrawText(text_placeholder, 0, y_offset - text_height/2);
		gc->DrawText(text_placeholder, w - text_width, y_offset - text_height/2);
		text_width += 20;
	}
	wxPen pen{wxColour{0ul}, 1};
	gc->SetPen(pen);
	gc->DrawRoundedRectangle(text_width, y_offset - slider_h/2, w - text_width*2, slider_h, 1);

	min_slider_path = gc->CreatePath();
	min_slider_path.AddCircle(text_width + w*min_pos, y_offset, 10);
	min_slider_path.MoveToPoint(text_width + w*min_pos, y_offset - 10);
	min_slider_path.AddLineToPoint(text_width + w*min_pos, y_offset - 20);
	min_slider_path.AddRectangle(text_width + w*min_pos - 25, y_offset - 35, 25, 15);

	max_slider_path = gc->CreatePath();
	max_slider_path.AddCircle(w*max_pos - text_width, y_offset, 10);
	max_slider_path.MoveToPoint(w*max_pos - text_width, y_offset - 10);
	max_slider_path.AddLineToPoint(w*max_pos - text_width, y_offset - 20);
	max_slider_path.AddRectangle(w*max_pos - text_width, y_offset - 35, 25, 15);

	wxBrush brush{wxColour{0xFFCCCC}};
	gc->SetBrush(brush);
	pen.SetWidth(2);
	pen.SetColour(wxColour{0x008800});
	gc->SetPen(pen);
	gc->DrawPath(min_slider_path);
	pen.SetColour(wxColour{0x000088});
	gc->SetPen(pen);
	gc->DrawPath(max_slider_path);
}