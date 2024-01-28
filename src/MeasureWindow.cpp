#include "MeasureWindow.h"
#include "Frame.h"

wxWindowID
	MeasureWindow::slider_algorithm_id = wxWindow::NewControlId(),
	MeasureWindow::slider_transparency_id = wxWindow::NewControlId(),
	MeasureWindow::button_color_id = wxWindow::NewControlId(),
	MeasureWindow::button_erosion_id = wxWindow::NewControlId(),
	MeasureWindow::button_dilation_id = wxWindow::NewControlId(),
	MeasureWindow::checkbox_color_id = wxWindow::NewControlId(),
	MeasureWindow::checkbox_background_id = wxWindow::NewControlId();

wxBEGIN_EVENT_TABLE(MeasureWindow, wxWindow)
    EVT_COMMAND_SCROLL_CHANGED(MeasureWindow::slider_algorithm_id, MeasureWindow::OnChangeDifference)
	EVT_COMMAND_SCROLL_CHANGED(MeasureWindow::slider_transparency_id, MeasureWindow::OnChangeTransparency)
    EVT_BUTTON(MeasureWindow::button_color_id, MeasureWindow::OnChangeColor)
	EVT_BUTTON(MeasureWindow::button_erosion_id, MeasureWindow::OnErosion)
	EVT_BUTTON(MeasureWindow::button_dilation_id, MeasureWindow::OnDilation)
	EVT_CHECKBOX(MeasureWindow::checkbox_color_id, MeasureWindow::OnSwitchColor)
	EVT_CHECKBOX(MeasureWindow::checkbox_background_id, MeasureWindow::OnDeleteBackground)
wxEND_EVENT_TABLE()

void MeasureWindow::OnSwitchColor(wxCommandEvent& event)
{
	update_image();
}

void MeasureWindow::OnDeleteBackground(wxCommandEvent& event)
{
	uint32_t id_to_delete = get_biggest_pore_id();
	if (event.IsChecked())
		m_deleted_pores.insert(id_to_delete);
	else
		if (m_deleted_pores.erase(id_to_delete) == 0)
			return;
	update_image();
}

void MeasureWindow::OnChangeColor(wxCommandEvent& event)
{
	m_colors.erase(m_colors.begin() + 3*m_pores.get<tag_hashset>().size(), m_colors.end());
	for (uint8_t& channel: m_colors)
		channel = uint8_t(m_random(m_rand_engine));
	update_image();
}

void MeasureWindow::OnChangeDifference(wxScrollEvent& event)
{
	if (auto view = gil::view(ImageWindow::image_current); !view.empty())
		Measure(view.xy_at(0, 0));
	update_image();
}

void MeasureWindow::OnChangeTransparency(wxScrollEvent& event)
{
	update_image();
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

	m_deleted_pores.clear();
	if (m_checkBox_background->IsChecked())
		m_deleted_pores.insert(get_biggest_pore_id());

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

void MeasureWindow::OnErosion(wxCommandEvent& event)
{
	using structure_t = std::vector<std::pair<int16_t, int16_t>>;
	structure_t structure{{0, 0}, {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};

	struct eroser
	{
		MeasureWindow* self;
		const structure_t& structure;
		decltype(m_pores) erosed_pores;
		decltype(m_pores)::iterator it, end;
		decltype(m_pores)::node_type node;

		eroser(MeasureWindow* self, structure_t& structure) : self(self), structure(structure), it(self->m_pores.get<tag_multiset>().begin()), end(self->m_pores.get<tag_multiset>().end())
		{
			for (std::vector<decltype(it)> pixels_to_erase;;)
			{
				uint32_t pore_id = it->first;
				do
				{
					for (auto& pair: structure)
					{
						if (std::ptrdiff_t coord_x = it->second.first + pair.first, coord_y = it->second.second + pair.second;
							coord_x >= 0 && coord_x < self->width || coord_y >= 0 || coord_y < self->height)
						{
							if (auto pore_pixel_it = self->m_pores.get<tag_hashset>().find(coord_t{coord_x, coord_y});
								pore_pixel_it == self->m_pores.get<tag_hashset>().end() || pore_pixel_it->first != pore_id)
							{
								pixels_to_erase.push_back(it);
								break;
							}
						}
					}
					if (++it == end) [[unlikely]]
					{
						for (auto& it_erase: pixels_to_erase)
							self->m_pores.get<tag_multiset>().erase(it_erase);
						separate_pores();
						return;
					}
				} while (it->first == pore_id);
			}
		}
		void separate_pores()
		{
			it = self->m_pores.get<tag_multiset>().begin();
			decltype(erosed_pores)::iterator it_erosed, end_erosed = erosed_pores.get<tag_multiset>().end();
			auto inspect_neighbours = [this, &it_erosed](std::ptrdiff_t dx, std::ptrdiff_t dy)
			{
				if (auto near_it = self->m_pores.get<tag_hashset>().find(coord_t{it_erosed->second.first + dx, it_erosed->second.second + dy});
					near_it != self->m_pores.get<tag_hashset>().end())
				{
					node = self->m_pores.get<tag_hashset>().extract(near_it);
					node.value().first = self->pores_count;
					erosed_pores.get<tag_hashset>().insert(std::move(node));
				}
			};

			self->pores_count = 0;
			for (;;)
			{
				if (uint32_t deleted_id = it->first; self->m_deleted_pores.contains(deleted_id)) [[unlikely]]
				{
					++self->pores_count;
					do
					{
						node = self->m_pores.get<tag_multiset>().extract(it++);
						node.value().first = self->pores_count;
						erosed_pores.get<tag_hashset>().insert(std::move(node));
						if (it == end) [[unlikely]]
							return;
					} while (it->first == deleted_id);
					continue;
				}
				node = self->m_pores.get<tag_multiset>().extract(it);
				node.value().first = ++self->pores_count;
				it_erosed = erosed_pores.get<tag_multiset>().insert(std::move(node)).position;
				do
				{
					inspect_neighbours(0, 1);
					inspect_neighbours(1, 1);
					inspect_neighbours(1, 0);
					inspect_neighbours(1, -1);
					inspect_neighbours(0, -1);
					inspect_neighbours(-1, -1);
					inspect_neighbours(-1, 0);
					inspect_neighbours(-1, 1);
				} while (++it_erosed != end_erosed);
				it = self->m_pores.get<tag_multiset>().begin();
				if (it == end) [[unlikely]]
					return;
			}
		}
		~eroser()
		{
			self->m_pores = std::move(erosed_pores);
			self->m_deleted_pores.clear();
			if (self->m_checkBox_background->IsChecked())
				self->m_deleted_pores.insert(self->get_biggest_pore_id());
			self->update_image();
		}
	} __(this, structure);
}

void MeasureWindow::OnDilation(wxCommandEvent& event)
{
	;
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

uint32_t MeasureWindow::get_biggest_pore_id()
{
	uint32_t id_to_delete = 0;
	for (uint32_t i = 0, max = m_pores.get<tag_multiset>().rbegin()->first, max_count = 0; i <= max; ++i)
		if (uint32_t c = m_pores.get<tag_multiset>().count(i); c > max_count) [[unlikely]]
		{
			max_count = c;
			id_to_delete = i;
		}
	return id_to_delete;
}

void MeasureWindow::update_image()
{
	ImageWindow* iw = static_cast<Frame*>(GetParent())->m_image;
	iw->Refresh();
	iw->Update();
}
