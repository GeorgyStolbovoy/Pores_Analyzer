#include "MeasureWindow.h"
#include "Frame.h"

wxWindowID
	MeasureWindow::slider_algorithm_id = wxWindow::NewControlId(),
	MeasureWindow::button_color_id = wxWindow::NewControlId(),
	MeasureWindow::checkbox_color_id = wxWindow::NewControlId();

wxBEGIN_EVENT_TABLE(MeasureWindow, wxWindow)
    EVT_COMMAND_SCROLL_CHANGED(MeasureWindow::slider_algorithm_id, MeasureWindow::OnChangeDifference)
    EVT_BUTTON(MeasureWindow::button_color_id, MeasureWindow::OnChangeColor)
	EVT_CHECKBOX(MeasureWindow::checkbox_color_id, MeasureWindow::OnSwitchColor)
wxEND_EVENT_TABLE()

#define UPDATE_IMAGE() \
	ImageWindow* iw = static_cast<Frame*>(GetParent())->m_image; \
	iw->Refresh(); \
	iw->Update();

void MeasureWindow::OnSwitchColor(wxCommandEvent& event)
{
	UPDATE_IMAGE()
}

void MeasureWindow::OnChangeColor(wxCommandEvent& event)
{
	m_colors.erase(m_colors.begin() + 3*m_pores.get<tag_hashset>().size(), m_colors.end());
	for (uint8_t& channel: m_colors)
		channel = uint8_t(m_random(m_rand_engine));
	UPDATE_IMAGE()
}

void MeasureWindow::OnChangeDifference(wxScrollEvent& event)
{
	if (auto view = gil::view(ImageWindow::image_current); !view.empty())
		Measure(view.xy_at(0, 0));
	UPDATE_IMAGE()
}

void MeasureWindow::Measure(locator_t&& loc)
{
	pores_count = 0;
	diff = uint8_t(m_slider_algorithm->GetValue());
	m_pores.clear();
	
	inspect_pore({std::move(loc), {0, 0}});
	for (auto chk_it = m_checklist.get<tag_sequenced>().begin(); chk_it != m_checklist.get<tag_sequenced>().end(); ++chk_it)
	{
		if (!m_pores.get<tag_hashset>().contains(chk_it->second)) [[unlikely]]
			inspect_pore(*chk_it);
	}
	m_checklist.clear();

	uint32_t color_num = 3*m_pores.get<tag_hashset>().size();
	if (std::size_t size = m_colors.size(); size < color_num)
	{
		m_colors.reserve(color_num);
		for (uint32_t i = size; i < color_num; ++i)
			m_colors.push_back(uint8_t(m_random(m_rand_engine)));
	}
}

void MeasureWindow::NewMeasure()
{
	auto& view = gil::view(ImageWindow::image_current);
	auto loc = view.xy_at(0, 0);
#define CACHED_LOC(pos, dx, dy) cl_##pos = loc.cache_location(dx, dy)
	CACHED_LOC(lt, -1, -1);	CACHED_LOC(t, 0, -1);	CACHED_LOC(rt, 1, -1);
	CACHED_LOC(l, -1, 0);							CACHED_LOC(r, 1, 0);
	CACHED_LOC(lb, -1, 1);	CACHED_LOC(b, 0, 1);	CACHED_LOC(rb, 1, 1);
#undef CACHED_LOC
	height = view.height();
	width = view.width();
	Measure(std::move(loc));
}

void MeasureWindow::inspect_pore(const inspecting_pixel& insp_pixel)
{
	inspecting_pixels_t inspecting{{insp_pixel}}, local_checklist;
	float reference = *insp_pixel.first;
	uint32_t pore_num = ++pores_count, count = 0, sum = 0;

	for (auto insp_it = inspecting.get<tag_sequenced>().begin();;)
	{
		do
		{
			m_pores.get<tag_multiset>().insert({pore_num, insp_it->second});
			sum += *insp_it->first;
			++count;

			std::ptrdiff_t x = insp_it->second.first, y = insp_it->second.second;
			bool under_top = y > 0, above_bottom = y < height-1, before_left = x > 0, before_right = x < width-1;

#define INSPECT(cond, dx, dy, dir, opt_2nd_inspect) \
			if (cond) \
			{ \
				if (coord_t coord{x + dx, y + dy}; !m_pores.get<tag_hashset>().contains(coord)) \
				{ \
					if (inspecting_pixel px{insp_it->first + gil::point(dx, dy), coord}; std::abs(reference - insp_it->first[cl_##dir]) < diff) \
						inspecting.get<tag_hashset>().insert(std::move(px)); \
					else \
						local_checklist.get<tag_hashset>().insert(std::move(px)); \
				} \
				opt_2nd_inspect \
			}
			INSPECT(above_bottom, 0, 1, b, INSPECT(before_right, 1, 1, rb,))
			INSPECT(before_right, 1, 0, r, INSPECT(under_top, 1, -1, rt,))
			INSPECT(under_top, 0, -1, t, INSPECT(before_left, -1, -1, lt,))
			INSPECT(before_left, -1, 0, l, INSPECT(above_bottom, -1, 1, lb,))
#undef INSPECT
		} while(++insp_it != inspecting.get<tag_sequenced>().end());
		reference = sum / float(count);
		auto saved_pos_it = insp_it;
		--saved_pos_it;
		for (auto chk_it = local_checklist.get<tag_sequenced>().begin(), chk_end = local_checklist.get<tag_sequenced>().end(); chk_it != chk_end;)
		{
			if (!m_pores.get<tag_hashset>().contains(chk_it->second))
			{
				if (std::abs(reference - *chk_it->first) < diff) [[unlikely]]
					inspecting.get<tag_sequenced>().splice(insp_it, local_checklist.get<tag_sequenced>(), chk_it++);
				else
					++chk_it;
			}
		}
		if (++saved_pos_it != insp_it)
			insp_it = saved_pos_it;
		else
			break;
	}
	m_checklist.get<tag_hashset>().merge(local_checklist.get<tag_hashset>());
}