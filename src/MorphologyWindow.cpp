#include "MorphologyWindow.h"
#include "Utils.h"
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include "wx/msgdlg.h"
#include <memory>


#define INITIAL_STRUCTURE_OF(Type) Type{{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}}

// MorphologyWindow

wxBEGIN_EVENT_TABLE(MorphologyWindow, wxWindow)
    EVT_PAINT(MorphologyWindow::OnPaint)
    EVT_LEFT_UP(MorphologyWindow::OnMouseLeftUp)
wxEND_EVENT_TABLE()

MorphologyWindow::MorphologyWindow(wxWindow *parent)
	: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 128)), StructureHolder(INITIAL_STRUCTURE_OF(structure_t))
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxLIGHT_GREY);
    SetCursor(wxCursor{wxCURSOR_HAND});
}

void MorphologyWindow::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);
	dc.Clear();
	auto [line_count, cells_count] = calc_offset_and_get_lines_cells_count();
	std::unique_ptr<wxGraphicsContext> gc{wxGraphicsContext::Create(dc)};
	display(gc.get(), GetSize().GetWidth(), GetSize().GetHeight(), nearest_integer<int>(std::min(GetSize().GetWidth(), GetSize().GetHeight())/50.0f), line_count, cells_count);
}

void MorphologyWindow::OnMouseLeftUp(wxMouseEvent& event)
{
	MorphologyDialog dlg{this};
	dlg.ShowModal();
}

// MorphologyDialog

wxWindowIDRef
	MorphologyWindow::MorphologyDialog::button_reset_id = wxWindow::NewControlId(),
	MorphologyWindow::MorphologyDialog::button_ok_id = wxWindow::NewControlId(),
	MorphologyWindow::MorphologyDialog::button_cancel_id = wxWindow::NewControlId();

wxBEGIN_EVENT_TABLE(MorphologyWindow::MorphologyDialog, wxWindow)
	EVT_BUTTON(MorphologyWindow::MorphologyDialog::button_reset_id, MorphologyWindow::MorphologyDialog::OnReset)
	EVT_BUTTON(MorphologyWindow::MorphologyDialog::button_ok_id, MorphologyWindow::MorphologyDialog::OnOk)
	EVT_BUTTON(MorphologyWindow::MorphologyDialog::button_cancel_id, MorphologyWindow::MorphologyDialog::OnCancel)
	EVT_CLOSE(MorphologyWindow::MorphologyDialog::Exit)
wxEND_EVENT_TABLE()

MorphologyWindow::MorphologyDialog::MorphologyDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, wxT("Редактор структурных элементов"), wxDefaultPosition, wxSize(500,500), wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER)
{
	SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* sizer_main = new wxBoxSizer( wxVERTICAL );

	MorphologyWindow* mw = static_cast<MorphologyWindow*>(parent);
	m_struct_window = new StructureEditingWindow(this, StructureEditingWindow::structure_t{mw->structure.begin(), mw->structure.end()});

	sizer_main->Add( m_struct_window, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* sizer_buttons = new wxBoxSizer( wxHORIZONTAL );

	m_button_reset = new wxButton(this, button_reset_id, wxT("Сброс"));
	sizer_buttons->Add( m_button_reset, 1, wxALL, 5 );

	m_button_ok = new wxButton(this, button_ok_id, wxT("Применить"));
	sizer_buttons->Add( m_button_ok, 1, wxALL, 5 );

	m_button_cancel = new wxButton(this, button_cancel_id, wxT("Отмена"));
	sizer_buttons->Add( m_button_cancel, 1, wxALL, 5 );

	sizer_main->Add( sizer_buttons, 0, wxEXPAND, 5 );

	SetSizer( sizer_main );
	Layout();
	Centre( wxBOTH );
	SetBackgroundColour(COLOR_WINDOW);
	SetIcon({wxT("wxICON_AAA")});
}

void MorphologyWindow::MorphologyDialog::OnReset(wxCommandEvent& event)
{
	if (wxMessageDialog{this, wxT("Вы действительно хотите вернуть структурный элемент по умолчанию?"), wxT("Сброс"), wxYES_NO|wxICON_QUESTION}.ShowModal() == wxID_YES)
	{
		m_struct_window->structure = INITIAL_STRUCTURE_OF(StructureEditingWindow::structure_t);
		m_struct_window->current_ofs = 1;
		m_struct_window->Refresh();
		m_struct_window->Update();
	}
}

void MorphologyWindow::MorphologyDialog::OnOk(wxCommandEvent& event)
{
	if (m_struct_window->structure.size() == 1) [[unlikely]]
		wxMessageDialog{this, wxT("Структурный элемент из 1-го пикселя бесполезен"), wxT("Проблема"), wxOK|wxICON_EXCLAMATION}.ShowModal();
	else [[likely]]
	{
		MorphologyWindow* parent = static_cast<MorphologyWindow*>(GetParent());
		parent->structure.assign(m_struct_window->structure.begin(), m_struct_window->structure.end());
		parent->Refresh();
		parent->Update();
		EndModal(wxID_OK);
	}
}

void MorphologyWindow::MorphologyDialog::OnCancel(wxCommandEvent& event)
{
	if (wxMessageDialog{this, wxT("Вы действительно хотите выйти из редактора без сохранения структурного элемента?"), wxT("Отмена"), wxYES_NO|wxICON_QUESTION}.ShowModal() == wxID_YES)
		EndModal(wxID_CANCEL);
}

void MorphologyWindow::MorphologyDialog::Exit(wxCloseEvent& event)
{
	if (wxMessageDialog{this, wxT("Вы действительно хотите выйти из редактора без сохранения структурного элемента?"), wxT("Выход"), wxYES_NO|wxICON_QUESTION}.ShowModal() == wxID_YES)
		EndModal(wxID_CANCEL);
}

// StructureEditingWindow

wxBEGIN_EVENT_TABLE(MorphologyWindow::MorphologyDialog::StructureEditingWindow, wxWindow)
	EVT_PAINT(MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnPaint)
	EVT_MOTION(MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseMove)
	EVT_LEFT_DOWN(MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseLeftDown)
	EVT_LEFT_UP(MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseLeftUp)
	EVT_RIGHT_DOWN(MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseRightDown)
	EVT_RIGHT_UP(MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseRightUp)
	EVT_MOUSEWHEEL(MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseWheel)
	EVT_LEAVE_WINDOW(MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnLeave)
wxEND_EVENT_TABLE()

MorphologyWindow::MorphologyDialog::StructureEditingWindow::StructureEditingWindow(wxWindow *parent, structure_t&& structure)
	: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(500,500), wxFULL_REPAINT_ON_RESIZE), StructureHolder(std::move(structure))
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);
	dc.Clear();
	std::unique_ptr<wxGraphicsContext> gc{wxGraphicsContext::Create(dc)};
	auto [line_count, cells_count] = calc_offset_and_get_lines_cells_count(current_ofs);
	assert(max_ofs <= 32);
	int width = GetSize().GetWidth(), height = GetSize().GetHeight();
	line_width = nearest_integer<int>(std::min(width, height)/100.0f);
	display(gc.get(), width, height, line_width, line_count, cells_count);
	if (hover.has_value())
	{
		wxBrush brush{wxColour{200, 0, 0, 128}};
		gc->SetBrush(brush);
		gc->SetPen(wxNullPen);
		double cell_width, cell_height;
		calc_params(cell_width, cell_height);
		gc->DrawRectangle((hover.value().first - 0.5)*cell_width, (hover.value().second - 0.5)*cell_height, width/cells_count, height/cells_count);
	}
}

void MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseMove(wxMouseEvent& event)
{
	int width = GetSize().GetWidth(), height = GetSize().GetHeight();
	int x = event.GetX(), y = event.GetY();
	if (x < 0 || y < 0 || x >= width || y >= height) [[unlikely]]
		return;
	x -= 0.5*width;
	y = -y + 0.5*height;
	double cell_width, cell_height;
	int8_t ofs_x, ofs_y;
	calc_params(cell_width, cell_height, ofs_x, ofs_y, x, y);
	if (std::abs(cell_width*(ofs_x - 0.5) - x) > line_width/2.0f && std::abs(cell_height*(ofs_y - 0.5) - y) > line_width/2.0f && (ofs_x != 0 || ofs_y != 0))
	{
		structure_element element{ofs_x, ofs_y};
		if (auto find_it = structure.find(element); find_it == structure.end())
		{
			if (!hover.has_value() || hover.value().first != element.first || hover.value().second != element.second)
			{
				if (HasCapture())
				{
					if (add_or_remove_mode)
					{
						hover = std::nullopt;
						structure.insert(std::move(element));
						Refresh();
						Update();
						return;
					}
				}
				hover = std::move(element);
				Refresh();
				Update();
			}
		}
		else
		{
			if (HasCapture())
			{
				if (!add_or_remove_mode && (find_it->first != 0 || find_it->second != 0))
					structure.erase(find_it);
				else
					return;
			}
			else
				hover = std::nullopt;
			Refresh();
			Update();
		}
	}
	else if (hover.has_value())
	{
		hover = std::nullopt;
		Refresh();
		Update();
	}
}

void MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseLeftDown(wxMouseEvent& event)
{
	CaptureMouse();
	add_or_remove_mode = true;
	if (hover.has_value())
	{
		structure.insert(hover.value());
		hover = std::nullopt;
		Refresh();
		Update();
	}
}

void MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseLeftUp(wxMouseEvent& event)
{
	if (HasCapture())
		ReleaseMouse();
}

void MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseRightDown(wxMouseEvent& event)
{
	CaptureMouse();
	add_or_remove_mode = false;
	int x = event.GetX() - 0.5*GetSize().GetWidth(), y = -event.GetY() + 0.5*GetSize().GetHeight();
	double cell_width, cell_height;
	int8_t ofs_x, ofs_y;
	calc_params(cell_width, cell_height, ofs_x, ofs_y, x, y);
	if (auto find_it = structure.find({ofs_x, ofs_y}); find_it != structure.end())
	{
		structure.erase(find_it);
		Refresh();
		Update();
	}
}

void MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseRightUp(wxMouseEvent& event)
{
	if (HasCapture())
		ReleaseMouse();
}

void MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnMouseWheel(wxMouseEvent& event)
{
	if (event.GetWheelRotation() > 0)
	{
		if (current_ofs < 32)
			++current_ofs;
		else
			return;
	}
	else
		if (current_ofs > calc_offset())
			--current_ofs;
		else
			return;
	Refresh();
	Update();
}

void MorphologyWindow::MorphologyDialog::StructureEditingWindow::OnLeave(wxMouseEvent& event)
{
	hover = std::nullopt;
	Refresh();
	Update();
}

void MorphologyWindow::MorphologyDialog::StructureEditingWindow::calc_params(double& cell_width, double& cell_height)
{
	float cells_num = max_ofs < 32 ? 2*max_ofs + 3 : 65;
	cell_width = GetSize().GetWidth() / cells_num;
	cell_height = GetSize().GetHeight() / cells_num;
}

void MorphologyWindow::MorphologyDialog::StructureEditingWindow::calc_params(double& cell_width, double& cell_height, int8_t& ofs_x, int8_t& ofs_y, int x, int y)
{
	calc_params(cell_width, cell_height);
	ofs_x = nearest_integer<int8_t>(std::floor(x / cell_width + 0.5));
	ofs_y = nearest_integer<int8_t>(std::floor(y / cell_height + 0.5));
}

// StructureHolder

template <template <class... T> class S, class... Ys>
int8_t StructureHolder<S, Ys...>::calc_offset()
{
	int8_t ofs = 1;
	for (auto [dx, dy]: structure)
		if (int8_t d = std::abs(dx); d > ofs) [[unlikely]]
			ofs = d;
		else if (d = std::abs(dy); d > ofs) [[unlikely]]
			ofs = d;
	return ofs;
}

template <template <class... T> class S, class... Ys>
std::pair<double, double> StructureHolder<S, Ys...>::calc_offset_and_get_lines_cells_count(int8_t default_ofs)
{
	max_ofs = std::max(calc_offset(), default_ofs);
	double line_count = 2*max_ofs + 2, cells_count = 2*max_ofs + 3;
	if (max_ofs >= 32) [[unlikely]]
	{
		line_count = 64;
		cells_count = 65;
	}
	return {line_count, cells_count};
}

template <template <class... T> class S, class... Ys>
void StructureHolder<S, Ys...>::display(wxGraphicsContext* gc, int width, int height, int line_width, double line_count, double cells_count)
{
	gc->SetTransform(gc->CreateMatrix(1, 0, 0, -1, 0.5*width, 0.5*height));
	wxBrush brush{wxColour{0x004400}};
	gc->SetBrush(brush);
	gc->DrawRectangle(-0.5*width/cells_count, -0.5*height/cells_count, width/cells_count, height/cells_count);
	brush.SetColour(wxColour{0ul});
	gc->SetBrush(brush);
	for (auto [dx, dy]: structure)
	{
		gc->DrawRectangle((dx - 0.5)*width/cells_count, (dy - 0.5)*height/cells_count, width/cells_count, height/cells_count);
	}
	line_width -= (line_width*line_count) / (width/4.0f);
	if (line_width > 0)
	{
		wxPen pen{wxColour{0x00aa00}, line_width};
		gc->SetPen(pen);
		for (int8_t i = 1; i <= line_count; ++i)
		{
			double x = -0.5*width + i*width/cells_count, y = 0.5*height - i*height/cells_count;
			gc->StrokeLine(-0.5*width, y, 0.5*width, y);
			gc->StrokeLine(x, 0.5*height, x, -0.5*height);
		}
	}
}