#include "MeasureWindow.h"
#include "Frame.h"

wxWindowID
	MeasureWindow::collapse_morphology_id = wxWindow::NewControlId(3),
	MeasureWindow::collapse_color_id = MeasureWindow::collapse_morphology_id + 1,
	MeasureWindow::collapse_filter_id = MeasureWindow::collapse_morphology_id + 2,
	MeasureWindow::slider_algorithm_id = wxWindow::NewControlId(),
	MeasureWindow::slider_transparency_id = wxWindow::NewControlId(),
	MeasureWindow::button_color_id = wxWindow::NewControlId(),
	MeasureWindow::button_erosion_id = wxWindow::NewControlId(),
	MeasureWindow::button_dilation_id = wxWindow::NewControlId(),
	MeasureWindow::toggle_color_id = wxWindow::NewControlId(),
	MeasureWindow::toggle_background_id = wxWindow::NewControlId(),
	MeasureWindow::toggle_boundaries_id = wxWindow::NewControlId();

wxBEGIN_EVENT_TABLE(MeasureWindow, wxWindow)
    EVT_COMMAND_SCROLL_CHANGED(MeasureWindow::slider_algorithm_id, MeasureWindow::OnChangeDifference)
	EVT_COMMAND_SCROLL_CHANGED(MeasureWindow::slider_transparency_id, MeasureWindow::OnChangeTransparency)
    EVT_BUTTON(MeasureWindow::button_color_id, MeasureWindow::OnChangeColor)
	EVT_BUTTON(MeasureWindow::button_erosion_id, MeasureWindow::OnErosion)
	EVT_BUTTON(MeasureWindow::button_dilation_id, MeasureWindow::OnDilation)
	EVT_TOGGLEBUTTON(MeasureWindow::toggle_color_id, MeasureWindow::OnSwitchColor)
	EVT_TOGGLEBUTTON(MeasureWindow::toggle_background_id, MeasureWindow::OnDeleteBackground)
	EVT_COLLAPSIBLEPANE_CHANGED(wxID_ANY, MeasureWindow::OnCollapse)
wxEND_EVENT_TABLE()

void MeasureWindow::OnSwitchColor(wxCommandEvent& event)
{
	update_image<false>();
}

void MeasureWindow::OnDeleteBackground(wxCommandEvent& event)
{
	uint32_t id_to_delete = get_biggest_pore_id();
	if (event.IsChecked())
	{
		if (m_deleted_pores.insert(id_to_delete).second)
		{
			if (m_selected_pores.contains(id_to_delete)) [[unlikely]]
			{
				parent_frame->m_image->m_sel_session.value().deselect(this, id_to_delete);
				parent_frame->m_statistic->pores_statistic_list->deselect_item(id_to_delete);
				if (m_selected_pores.empty())
					parent_frame->m_image->m_sel_session = std::nullopt;
			}
			parent_frame->m_statistic->pores_statistic_list->on_pore_deleted(id_to_delete);
		}
		else
			return;
	}
	else
	{
		if (m_deleted_pores.erase(id_to_delete) > 0)
			parent_frame->m_statistic->pores_statistic_list->on_pore_recovered(id_to_delete);
		else
			return;
	}
	update_image<false>();
}

void MeasureWindow::OnChangeColor(wxCommandEvent& event)
{
	for (uint8_t& channel: m_colors)
		channel = uint8_t(m_random(m_rand_engine));
	update_image<false>();
}

void MeasureWindow::OnChangeDifference(wxScrollEvent& event)
{
	if (auto view = gil::view(parent_frame->m_image->image); !view.empty())
	{
		Measure(view.xy_at(0, 0));
		update_image<true>();
	}
}

void MeasureWindow::OnChangeTransparency(wxScrollEvent& event)
{
	update_image<false>();
}

void MeasureWindow::OnCollapse(wxCollapsiblePaneEvent& event)
{
	if (wxWindow* pane = FindWindowById(event.GetId()); !event.GetCollapsed())
	{
		if (!pane_sizer->IsEmpty())
		{
			wxWindow* item = pane_sizer->GetItem(std::size_t(0))->GetWindow();
			static_cast<wxCollapsiblePane*>(item)->Collapse();
			pane_sizer->Detach(item);
			collapses_sizer->Insert(item->GetId() - collapse_morphology_id, item, 1, wxEXPAND, 5);
		}
		collapses_sizer->Detach(pane);
		pane_sizer->Add(pane, 1, wxEXPAND, 5);
	}
	else
	{
		pane_sizer->Detach(pane);
		collapses_sizer->Insert(pane->GetId() - collapse_morphology_id, pane, 1, wxEXPAND, 5);
	}
	parent_frame->Layout();
}

void MeasureWindow::OnErosion(wxCommandEvent& event)
{
	struct eroser
	{
		MeasureWindow* self;
		pores_container erosed_pores;
		pores_container::iterator it, end;
		pores_container::node_type node;
		std::unordered_set<
			pores_container::iterator,
			decltype([](const auto& it){return boost::hash<coord_t>{}(it->second);}),
			decltype([](const auto& lhs, const auto& rhs){return lhs->second.first == rhs->second.first && lhs->second.second == rhs->second.second;})
		> pixels_to_erase;

		eroser(MeasureWindow* self) : self(self), it(self->m_pores.get<tag_multiset>().begin()), end(self->m_pores.get<tag_multiset>().end())
		{
			for (;;)
			{
				uint32_t pore_id = it->first;
				if (!self->m_deleted_pores.contains(pore_id))
				{
					do
					{
						for (auto& pair: self->m_window_erosion->structure)
						{
							if (std::ptrdiff_t coord_x = it->second.first + pair.first, coord_y = it->second.second + pair.second;
								coord_x >= 0 && coord_x < self->width && coord_y >= 0 && coord_y < self->height)
							{
								if (auto pore_pixel_it = self->m_pores.get<tag_hashset>().find(coord_t{coord_x, coord_y});
									pore_pixel_it == self->m_pores.get<tag_hashset>().end() || pore_pixel_it->first != pore_id)
								{
									pixels_to_erase.insert(it);
									break;
								}
							}
						}
						if (++it == end) [[unlikely]]
						{
							separate_pores();
							return;
						}
					} while (it->first == pore_id);
				}
				else if ((it = self->m_pores.get<MeasureWindow::tag_multiset>().lower_bound(pore_id, [](uint32_t lhs, uint32_t rhs) {return lhs <= rhs;}))
					== self->m_pores.get<MeasureWindow::tag_multiset>().end()) [[unlikely]]
				{
					separate_pores();
					return;
				}
			}
		}
		void separate_pores()
		{
			for (auto& it_erase: pixels_to_erase)
				self->m_pores.get<tag_multiset>().erase(it_erase);
			it = self->m_pores.get<tag_multiset>().begin();
			pores_container::iterator it_erosed, end_erosed = erosed_pores.get<tag_multiset>().end();
			uint32_t old_id = 0;
			auto inspect_neighbour = [this, &it_erosed, &old_id](std::ptrdiff_t dx, std::ptrdiff_t dy)
			{
				if (auto near_it = self->m_pores.get<tag_hashset>().find(coord_t{it_erosed->second.first + dx, it_erosed->second.second + dy});
					near_it != self->m_pores.get<tag_hashset>().end() && near_it->first == old_id)
				{
					node = self->m_pores.get<tag_hashset>().extract(near_it);
					node.value().first = self->pores_count;
					erosed_pores.get<tag_hashset>().insert(std::move(node));
				}
			};

			self->pores_count = 0;
			for (;;)
			{
				old_id = it->first;
				if (self->m_deleted_pores.contains(old_id)) [[unlikely]]
				{
					++self->pores_count;
					do
					{
						node = self->m_pores.get<tag_multiset>().extract(it++);
						node.value().first = self->pores_count;
						erosed_pores.get<tag_hashset>().insert(std::move(node));
						if (it == end) [[unlikely]]
							return;
					} while (it->first == old_id);
					continue;
				}
				node = self->m_pores.get<tag_multiset>().extract(it);
				node.value().first = ++self->pores_count;
				it_erosed = erosed_pores.get<tag_multiset>().insert(std::move(node)).position;
				do
				{
					inspect_neighbour(0, 1);
					inspect_neighbour(1, 1);
					inspect_neighbour(1, 0);
					inspect_neighbour(1, -1);
					inspect_neighbour(0, -1);
					inspect_neighbour(-1, -1);
					inspect_neighbour(-1, 0);
					inspect_neighbour(-1, 1);
				} while (++it_erosed != end_erosed);
				it = self->m_pores.get<tag_multiset>().begin();
				if (it == end) [[unlikely]]
					return;
			}
		}
		~eroser()
		{
			self->m_pores = std::move(erosed_pores);
			self->after_measure();
			self->update_image<true>();
		}
	} __(this);
}

void MeasureWindow::OnDilation(wxCommandEvent& event)
{
	std::unordered_set<
		pore_coord_t,
		decltype([](const auto& pore){return boost::hash<coord_t>{}(pore.second);}),
		decltype([](const auto& lhs, const auto& rhs){return lhs.second.first == rhs.second.first && lhs.second.second == rhs.second.second;})
	> pixels_to_insert;
	auto finalize = [this, &pixels_to_insert]()
	{
		for (auto& pore: pixels_to_insert)
		{
			if (auto result = m_pores.get<tag_hashset>().insert(pore); !result.second && m_deleted_pores.contains(result.first->first)) [[unlikely]]
				m_pores.get<tag_multiset>().modify_key(m_pores.project<tag_multiset>(result.first), [&pore](uint32_t& id){id = pore.first;});
		}
		after_measure();
		update_image<true>();
	};

	for (auto it = m_pores.get<tag_multiset>().begin(), end = m_pores.get<tag_multiset>().end();;)
	{
		uint32_t pore_id = it->first;
		if (!m_deleted_pores.contains(pore_id))
		{
			do
			{
				for (auto& pair: m_window_dilation->structure)
				{
					if (std::ptrdiff_t coord_x = it->second.first + pair.first, coord_y = it->second.second + pair.second;
						coord_x >= 0 && coord_x < width && coord_y >= 0 && coord_y < height)
					{
						pixels_to_insert.insert({pore_id, coord_t{coord_x, coord_y}});
					}
				}
				if (++it == end) [[unlikely]]
				{
					finalize();
					return;
				}
			} while (it->first == pore_id);
		}
		else if ((it = m_pores.get<MeasureWindow::tag_multiset>().lower_bound(pore_id, [](uint32_t lhs, uint32_t rhs) {return lhs <= rhs;}))
			== m_pores.get<MeasureWindow::tag_multiset>().end()) [[unlikely]]
		{
			finalize();
			return;
		}
	}
}

void MeasureWindow::NewMeasure(ImageWindow::Image_t::view_t view)
{
	auto loc = view.xy_at(0, 0);
#define CACHED_LOC(pos, dx, dy) cl_##pos = loc.cache_location(dx, dy)
	CACHED_LOC(lt, -1, -1);	CACHED_LOC(t, 0, -1);	CACHED_LOC(rt, 1, -1);
	CACHED_LOC(l, -1, 0);							CACHED_LOC(r, 1, 0);
	CACHED_LOC(lb, -1, 1);	CACHED_LOC(b, 0, 1);	CACHED_LOC(rb, 1, 1);
#undef CACHED_LOC
	height = view.height();
	width = view.width();
	Measure(loc);
}

void MeasureWindow::Measure(locator_t loc)
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

	after_measure();
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
	for (uint32_t i = 1, max_count = 0; i <= pores_count; ++i)
		if (uint32_t c = m_pores.get<tag_multiset>().count(i); c > max_count) [[unlikely]]
		{
			max_count = c;
			id_to_delete = i;
		}
	return id_to_delete;
}

template <bool reset_selection>
void MeasureWindow::update_image()
{
	ImageWindow* iw = parent_frame->m_image;
	iw->marked_image = wxNullImage;
	if constexpr (reset_selection)
		iw->m_sel_session = std::nullopt;
	iw->Refresh();
	iw->Update();
}

void MeasureWindow::after_measure()
{
	m_selected_pores.clear();
	m_boundary_pixels.clear();
	m_deleted_pores.clear();
	if (m_toggle_background->GetValue())
		m_deleted_pores.insert(get_biggest_pore_id());

	uint32_t color_num = 3*pores_count;
	if (std::size_t size = m_colors.size(); size < color_num)
	{
		m_colors.reserve(color_num);
		for (uint32_t i = size; i < color_num; ++i)
			m_colors.push_back(uint8_t(m_random(m_rand_engine)));
	}
	else if (size > color_num)
		m_colors.erase(m_colors.begin() + color_num, m_colors.end());

	auto pore_it = m_pores.get<MeasureWindow::tag_multiset>().begin(), pore_end = m_pores.get<MeasureWindow::tag_multiset>().end();
	for (uint32_t i = pore_it->first;;)
	{
		if (MeasureWindow::pores_container::index<MeasureWindow::tag_hashset>::type::iterator end_it = m_pores.get<MeasureWindow::tag_hashset>().end(), tmp_it;
			pore_it->second.first == 0 || pore_it->second.first == width-1 || pore_it->second.second == 0 || pore_it->second.second == height-1 ||
#define COND(dx, dy) (tmp_it = m_pores.get<MeasureWindow::tag_hashset>().find(coord_t{pore_it->second.first dx, pore_it->second.second dy})) == end_it || tmp_it->first != i
			COND(, +1) || COND(+1, +1) || COND(+1,) || COND(+1, -1) || COND(, -1) || COND(-1, -1) || COND(-1,) || COND(-1, +1)) [[unlikely]]
			m_boundary_pixels.insert(pore_it->second);
		if (++pore_it == pore_end) [[unlikely]]
			break;
		if (pore_it->first != i) [[unlikely]]
			i = pore_it->first;
	}

	parent_frame->m_statistic->CollectStatistic();
}