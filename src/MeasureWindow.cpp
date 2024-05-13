#include "MeasureWindow.h"
#include "Frame.h"
#include <forward_list>

#define DECLARE_SLIDER_ID(z, data, s) MeasureWindow::s = wxWindow::NewControlId()
wxWindowID
	MeasureWindow::collapse_segmentation_id = wxWindow::NewControlId(3),
	MeasureWindow::collapse_color_id = MeasureWindow::collapse_segmentation_id + 1,
	MeasureWindow::collapse_filter_id = MeasureWindow::collapse_segmentation_id + 2,
	MeasureWindow::slider_amount_id = wxWindow::NewControlId(),
	MeasureWindow::slider_transparency_id = wxWindow::NewControlId(),
	MeasureWindow::slider_thres_id = wxWindow::NewControlId(),
	MeasureWindow::button_color_id = wxWindow::NewControlId(),
	MeasureWindow::toggle_color_id = wxWindow::NewControlId(),
	MeasureWindow::toggle_background_id = wxWindow::NewControlId(),
	BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(DECLARE_SLIDER_ID, ~, SLIDERS_IDS));

wxBEGIN_EVENT_TABLE(MeasureWindow, wxWindow)
	EVT_COMMAND_SCROLL_CHANGED(MeasureWindow::slider_thres_id, MeasureWindow::OnChangeThreshold)
    EVT_COMMAND_SCROLL_CHANGED(MeasureWindow::slider_amount_id, MeasureWindow::OnChangeSeparationAmount)
	EVT_COMMAND_SCROLL_CHANGED(MeasureWindow::slider_transparency_id, MeasureWindow::OnChangeTransparency)
    EVT_BUTTON(MeasureWindow::button_color_id, MeasureWindow::OnChangeColor)
	EVT_TOGGLEBUTTON(MeasureWindow::toggle_color_id, MeasureWindow::OnSwitchColor)
	EVT_COLLAPSIBLEPANE_CHANGED(wxID_ANY, MeasureWindow::OnCollapse)
wxEND_EVENT_TABLE()

void MeasureWindow::OnSwitchColor(wxCommandEvent& event)
{
	Frame::frame->m_image->update_image(false);
}

void MeasureWindow::OnChangeColor(wxCommandEvent& event)
{
	for (uint8_t& channel: m_colors)
		channel = uint8_t(m_random(m_rand_engine));
	Frame::frame->m_image->update_image(false);
}

void MeasureWindow::OnChangeTransparency(wxScrollEvent& event)
{
	Frame::frame->m_image->update_image(false);
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
			collapses_sizer->Insert(item->GetId() - collapse_segmentation_id, item, 1, wxEXPAND, 5);
		}
		collapses_sizer->Detach(pane);
		pane_sizer->Add(pane, 1, wxEXPAND, 5);
	}
	else
	{
		pane_sizer->Detach(pane);
		collapses_sizer->Insert(pane->GetId() - collapse_segmentation_id, pane, 1, wxEXPAND, 5);
	}
	Frame::frame->Layout();
}

void MeasureWindow::OnChangeThreshold(wxScrollEvent& event)
{
	static bool bug_workaround = true;
	if (bug_workaround)
	{
		if (std::exchange(threshold, m_slider_thres->GetValue()) == threshold)
			bug_workaround = false;
		if (auto view = gil::view(Frame::frame->m_image->image); !view.empty())
		{
			Segmentation();
			Frame::frame->m_image->update_image(true);
		}
	}
	else
		bug_workaround = true;
}

void MeasureWindow::OnChangeSeparationAmount(wxScrollEvent& event)
{
	static bool bug_workaround = true;
	if (bug_workaround)
	{
		if (std::exchange(amount, m_slider_amount->GetValue()) == amount)
			bug_workaround = false;
		if (auto view = gil::view(Frame::frame->m_image->image); !view.empty())
		{
			Segmentation();
			Frame::frame->m_image->update_image(true);
		}
	}
	else
		bug_workaround = true;
}

void MeasureWindow::NewMeasure(ImageWindow::Image_t::view_t view)
{
	ImageWindow::Image_t::view_t::locator loc = view.pixels();
#define CACHED_LOC(pos, dx, dy) cl_##pos = loc.cache_location(dx, dy)
	CACHED_LOC(lt, -1, -1);	CACHED_LOC(t, 0, -1);	CACHED_LOC(rt, 1, -1);
	CACHED_LOC(l, -1, 0);							CACHED_LOC(r, 1, 0);
	CACHED_LOC(lb, -1, 1);	CACHED_LOC(b, 0, 1);	CACHED_LOC(rb, 1, 1);
#undef CACHED_LOC
	height = view.height();
	width = view.width();
	Segmentation();
}

void MeasureWindow::Segmentation()
{
	using coords_t = std::unordered_set<coord_t, boost::hash<coord_t>>;

	m_pores.clear();

	auto image_view = gil::view(Frame::frame->m_image->image);
	std::forward_list<ImageWindow::Image_t::view_t::iterator> inspecting;
	coords_t coords, background;
	coords_t::iterator find_it, end_it = coords.end();
	auto first_pixel = image_view.at(0, 0);
	bool is_higher_thres = *first_pixel > threshold;

	for (std::ptrdiff_t i = 1; i < width; ++i)
		coords.emplace(i, 0);
	for (std::ptrdiff_t i = 0; i < width; ++i)
		for (std::ptrdiff_t j = 1; j < height; ++j)
			coords.emplace(i, j);
	inspecting.push_front(first_pixel);
	for (auto it = inspecting.begin(), end = inspecting.end();;)
	{
		coord_t coord{it->x_pos(), it->y_pos()};
		background.insert(coord);

#define INSPECT_IMPL(r, d, ofs) \
			if (find_it = coords.find({coord.first + BOOST_PP_TUPLE_ELEM(0, ofs), coord.second + BOOST_PP_TUPLE_ELEM(1, ofs)}); find_it != end_it) \
			{ \
				if (auto iter = *it - (-BOOST_PP_CAT(cl_, BOOST_PP_TUPLE_ELEM(2, ofs))); *iter > threshold == is_higher_thres) \
				{ \
					inspecting.emplace_after(it, iter); \
					coords.erase(find_it); \
				} \
			}
#define INSPECT() BOOST_PP_SEQ_FOR_EACH(INSPECT_IMPL, ~, ((0, 1, b))((1, 1, rb))((1, 0, r))((1, -1, rt))((0, -1, t))((-1, -1, lt))((-1, 0, l))((-1, 1, lb)))

		INSPECT()
		if (++it == end) [[unlikely]]
		{
			if (auto coord_it = coords.begin(); coord_it != coords.end())
			{
				first_pixel = image_view.at(coord_it->first, coord_it->second);
				is_higher_thres = *first_pixel > threshold;
				coords.erase(coord_it);
				inspecting.clear();
				inspecting.push_front(first_pixel);
				it = inspecting.begin();
				end = inspecting.end();

				uint32_t square = background.size();
				coords_t coords_without_bg;
				for (coords_t possible_background;;)
				{
					coord.first = it->x_pos(); coord.second = it->y_pos();
					possible_background.insert(coord);
					INSPECT()
					if (++it == end) [[unlikely]]
					{
						if (possible_background.size() > square) [[unlikely]]
						{
							coords_without_bg.merge(std::move(background));
							background = std::move(possible_background);
							square = background.size();
						}
						else [[likely]]
							coords_without_bg.merge(std::move(possible_background));
						if (coord_it = coords.begin(); coord_it != coords.end())
						{
							first_pixel = image_view.at(coord_it->first, coord_it->second);
							is_higher_thres = *first_pixel > threshold;
							coords.erase(coord_it);
							inspecting.clear();
							inspecting.push_front(first_pixel);
							it = inspecting.begin();
							end = inspecting.end();
						}
						else
						{
							coords = std::move(coords_without_bg);
							break;
						}
					}
				}
			}
			break;
		}
#undef INSPECT
	}

	// Объединить все соседние кроме фона
	std::forward_list<coords_t> pores;

	auto iter_to_pore = pores.before_begin();
	for (auto coords_it = coords.begin(), coords_end = coords.end(); coords_it != coords_end; coords_it = coords.begin())
	{
		iter_to_pore = pores.emplace_after(iter_to_pore);
		std::forward_list inspecting_coords{iter_to_pore->insert(coords.extract(coords_it)).position};
		auto insp_it = inspecting_coords.begin(), insp_end = inspecting_coords.end();
		do
		{
#define INSPECT(r, d, ofs) \
				if (find_it = coords.find({(*insp_it)->first BOOST_PP_TUPLE_ELEM(0, ofs), (*insp_it)->second BOOST_PP_TUPLE_ELEM(1, ofs)}); find_it != coords_end) \
					inspecting_coords.insert_after(insp_it, iter_to_pore->insert(coords.extract(find_it)).position);
			BOOST_PP_SEQ_FOR_EACH(INSPECT, ~, ((+0, +1))((+1, +1))((+1, +0))((+1, -1))((+0, -1))((-1, -1))((-1, +0))((-1, +1)))
#undef INSPECT
		} while (++insp_it != insp_end);
	}

	// Определить слипшиеся поры
	class Separation
	{
		using pores_list_t = std::forward_list<coords_t>;
		struct Group
		{
			pores_list_t m_elemental_pores;
			std::forward_list<Group> m_subgroups;
			coords_t m_erosed;

			Group() = default;
			Group(Group&&) = default;
			Group& operator=(Group&& other) = default;

			std::optional<pores_list_t> filter_and_dilate(std::size_t source_size)
			{
				struct filtration
				{
					std::size_t pores_count, max_pore_size;
					float average_size;
					bool need_refilter;

					bool do_filtration(Group* node)
					{
						if (auto prev_it = node->m_subgroups.before_begin(), cur_it = std::next(prev_it), end_it = node->m_subgroups.end(); cur_it != end_it)
						{
							for (;;)
							{
								if (do_filtration(&*cur_it))
								{
									if (auto sg_pores_it = cur_it->m_elemental_pores.begin(), sg_pores_end = cur_it->m_elemental_pores.end(); sg_pores_it == sg_pores_end)
									{
										if (auto sg_sg_it = cur_it->m_subgroups.begin(), sg_sg_end = cur_it->m_subgroups.end(); sg_sg_it == sg_sg_end)
										{
											if (std::size_t new_size = cur_it->m_erosed.size();
												Frame::frame->m_measure->amount*new_size >= max_pore_size && Frame::frame->m_measure->amount*new_size >= average_size)
											{
												++pores_count;
												need_refilter = true;
												node->m_elemental_pores.emplace_front(std::move(cur_it->m_erosed));
												if (new_size > max_pore_size) [[unlikely]]
													max_pore_size = new_size;
											}
											else
												node->m_erosed.merge(std::move(cur_it->m_erosed));
											if (cur_it = node->m_subgroups.erase_after(prev_it); cur_it == end_it)
												break;
											continue;
										}
										else if (std::next(sg_sg_it) == sg_sg_end)
										{
											cur_it->m_erosed.merge(std::move(sg_sg_it->m_erosed));
											cur_it->m_elemental_pores = std::move(sg_sg_it->m_elemental_pores);
											cur_it->m_subgroups.splice_after(sg_sg_it, std::move(sg_sg_it->m_subgroups));
											cur_it->m_subgroups.pop_front();
										}
									}
									else if (std::next(sg_pores_it) == sg_pores_end && cur_it->m_subgroups.empty())
									{
										sg_pores_it->merge(std::move(cur_it->m_erosed));
										if (std::size_t new_size = sg_pores_it->size();
											Frame::frame->m_measure->amount*new_size >= max_pore_size && Frame::frame->m_measure->amount*new_size >= average_size)
										{
											node->m_elemental_pores.splice_after(node->m_elemental_pores.before_begin(), std::move(cur_it->m_elemental_pores));
											if (new_size > max_pore_size)
											{
												max_pore_size = new_size;
												need_refilter = true;
											}
										}
										else
										{
											--pores_count;
											need_refilter = true;
											node->m_erosed.merge(std::move(*sg_pores_it));
										}
										if (cur_it = node->m_subgroups.erase_after(prev_it); cur_it == end_it)
											break;
										continue;
									}
								}
								if (++cur_it == end_it)
									break;
								++prev_it;
							}
						}
						std::size_t entities_count = std::distance(node->m_elemental_pores.begin(), node->m_elemental_pores.end());
						for (auto prev_it = node->m_elemental_pores.before_begin(), cur_it = std::next(prev_it), end_it = node->m_elemental_pores.end(); cur_it != end_it;)
						{
							if (std::size_t s = Frame::frame->m_measure->amount*cur_it->size(); s < max_pore_size || s < average_size)
							{
								node->m_erosed.merge(std::move(*cur_it));
								cur_it = node->m_elemental_pores.erase_after(prev_it);
								--pores_count;
								need_refilter = true;
							}
							else
							{
								++prev_it;
								++cur_it;
								++entities_count;
							}
						}
						return entities_count < 2;
					}
				} filter{get_pores_count(), get_pore_max_size()};

				do
				{
					filter.need_refilter = false;
					filter.average_size = source_size/float(filter.pores_count);
					if (filter.do_filtration(this))
					{
						if (auto sg_pores_it = m_elemental_pores.begin(), sg_pores_end = m_elemental_pores.end(); sg_pores_it == sg_pores_end)
						{
							if (auto sg_sg_it = m_subgroups.begin(), sg_sg_end = m_subgroups.end(); sg_sg_it == sg_sg_end)
								return std::nullopt;
							else if (std::next(sg_sg_it) == sg_sg_end)
							{
								m_erosed.merge(std::move(sg_sg_it->m_erosed));
								m_elemental_pores = std::move(sg_sg_it->m_elemental_pores);
								m_subgroups.splice_after(sg_sg_it, std::move(sg_sg_it->m_subgroups));
								m_subgroups.pop_front();
							}
						}
						else if (std::next(sg_pores_it) == sg_pores_end && m_subgroups.empty())
							return std::nullopt;
					}
				} while (filter.need_refilter);

				return dilate();
			}

		private:
			std::size_t get_pore_max_size()
			{
				std::size_t ret;
				if (auto pores_it = m_elemental_pores.begin(), pores_end = m_elemental_pores.end(); pores_it != pores_end)
				{
					ret = pores_it->size();
					for (; ++pores_it != pores_end;)
						if (std::size_t s = pores_it->size(); s > ret)
							ret = s;
					for (auto& gr : m_subgroups)
						if (std::size_t s = gr.get_pore_max_size(); s > ret) [[unlikely]]
							ret = s;
				}
				else
				{
					auto groups_it = m_subgroups.begin(), groups_end = m_subgroups.end();
					ret = groups_it->get_pore_max_size();
					for (; ++groups_it != groups_it;)
						if (std::size_t s = groups_it->get_pore_max_size(); s > ret)
							ret = s;
				}
				return ret;
			}

			std::size_t get_pores_count()
			{
				std::size_t ret = std::distance(m_elemental_pores.begin(), m_elemental_pores.end());
				for (auto& sgr : m_subgroups)
					ret += sgr.get_pores_count();
				return ret;
			}

			pores_list_t dilate()
			{
				for (auto& sgr : m_subgroups)
					m_elemental_pores.splice_after(m_elemental_pores.before_begin(), sgr.dilate());

				do
				{
					std::unordered_multimap<coords_t*, coords_t::iterator> iters_to_dilate;
					auto erosed_it = m_erosed.begin(), erosed_end = m_erosed.end();
					do
					{
						for (auto elem_pores_it = m_elemental_pores.begin(), elem_pores_end = m_elemental_pores.end(), elem_pores_owner = elem_pores_end;;)
						{
#define CHECK_CONTAINS(r, d, i, ofs) elem_pores_it->contains({erosed_it->first BOOST_PP_TUPLE_ELEM(0, ofs), erosed_it->second BOOST_PP_TUPLE_ELEM(1, ofs)}) BOOST_PP_EXPR_IF(BOOST_PP_NOT_EQUAL(d, i), ||)
							if (BOOST_PP_SEQ_FOR_EACH_I(CHECK_CONTAINS, 7, ((+0, +1))((+1, +1))((+1, +0))((+1, -1))((+0, -1))((-1, -1))((-1, +0))((-1, +1))))
#undef CHECK_CONTAINS
							{
								if (elem_pores_owner == elem_pores_end)
									elem_pores_owner = elem_pores_it;
								else
								{
									m_erosed.erase(erosed_it++);
									break;
								}
							}
							if (++elem_pores_it == elem_pores_end)
							{
								if (elem_pores_owner != elem_pores_end)
									iters_to_dilate.emplace(&*elem_pores_owner, erosed_it);
								++erosed_it;
								break;
							}
						}
					} while (erosed_it != erosed_end);
					if (auto dilate_it = iters_to_dilate.begin(), dilate_end = iters_to_dilate.end(); dilate_it != dilate_end)
						do
							dilate_it->first->insert(m_erosed.extract(dilate_it->second));
						while (++dilate_it != dilate_end);
					else
						break;
				} while (!m_erosed.empty());

				return std::move(m_elemental_pores);
			}
		};

		static std::optional<Group> separate_to_group(coords_t& coords)
		{
			for (Group group;;)
			{
				std::vector<coords_t::iterator> iters_to_erosed;
				for (auto coords_it = coords.begin(), coords_end = coords.end(); coords_it != coords_end; ++coords_it)
				{
					if (!coords.contains({coords_it->first, coords_it->second + 1}) || !coords.contains({coords_it->first + 1, coords_it->second})
						|| !coords.contains({coords_it->first, coords_it->second - 1}) || !coords.contains({coords_it->first - 1, coords_it->second})) [[unlikely]]
						iters_to_erosed.push_back(coords_it);
				}
				if (iters_to_erosed.size() == coords.size()) [[unlikely]]
				{
					coords.merge(std::move(group.m_erosed));
					return std::nullopt;
				}
				for (auto it : iters_to_erosed)
					group.m_erosed.insert(coords.extract(it));

				for (auto coords_end = coords.end();;)
				{
					coords_t elemental_pore;
					elemental_pore.insert(coords.extract(coords.begin()));
					std::forward_list inspecting{elemental_pore.begin()};
					auto insp_it = inspecting.begin(), insp_end = inspecting.end();
					do
					{
#define INSPECT(r, d, ofs)			if (auto find_it = coords.find({(*insp_it)->first BOOST_PP_TUPLE_ELEM(0, ofs), (*insp_it)->second BOOST_PP_TUPLE_ELEM(1, ofs)}); find_it != coords_end) \
								inspecting.insert_after(insp_it, elemental_pore.insert(coords.extract(find_it)).position);
						BOOST_PP_SEQ_FOR_EACH(INSPECT, ~, ((+0, +1))((+1, +1))((+1, +0))((+1, -1))((+0, -1))((-1, -1))((-1, +0))((-1, +1)))
#undef INSPECT
					} while (++insp_it != insp_end);
					if (coords.empty())
					{
						if (group.m_elemental_pores.empty())
						{
							coords = std::move(elemental_pore);
							break;
						}
						group.m_elemental_pores.emplace_front(std::move(elemental_pore));
						auto elem_pores_end = group.m_elemental_pores.end(), prev_it = group.m_elemental_pores.before_begin(), next_it = std::next(prev_it);
#define ADVANCE_SEPARATION(chk) \
							if (auto opt_separated_pores = separate_to_group(*next_it); opt_separated_pores.has_value()) \
							{ \
								next_it = group.m_elemental_pores.erase_after(prev_it); \
								group.m_subgroups.push_front(std::move(opt_separated_pores.value())); \
								BOOST_PP_EXPR_IF(chk, if (next_it == elem_pores_end) \
									break;) \
							} \
							else \
							{ \
								++next_it; \
								BOOST_PP_EXPR_IF(chk, if (next_it == elem_pores_end) [[unlikely]] \
									break;) \
								++prev_it; \
							}

						ADVANCE_SEPARATION(0)
						for (;;)
							ADVANCE_SEPARATION(1)
#undef ADVANCE_SEPARATION
						return group;
					}
					group.m_elemental_pores.emplace_front(std::move(elemental_pore));
				}
			}
		}

	public:
		static std::optional<pores_list_t> separate(coords_t& coords)
		{
			std::size_t source_size = coords.size();
			if (auto opt_separated_pores = separate_to_group(coords); opt_separated_pores.has_value())
			{
				if (auto opt_true_pores = opt_separated_pores.value().filter_and_dilate(source_size); opt_true_pores.has_value())
					return opt_true_pores.value();
				else
				{
					coords = std::move(opt_separated_pores.value().m_erosed);
					return std::nullopt;
				}
			}
			return std::nullopt;
		}
	};

	pores_count = 0;
	for (coords_t& cds : pores)
	{
		if (auto opt_separated_pores = Separation::separate(cds); opt_separated_pores.has_value())
		{
			auto sep_pores_it = opt_separated_pores.value().begin(), sep_pores_end = opt_separated_pores.value().end();
			do
			{
				++pores_count;
				for (auto& coord : *sep_pores_it)
					m_pores.get<tag_hashset>().insert({pores_count, coord});
			} while (++sep_pores_it != sep_pores_end);
		}
		else
		{
			++pores_count;
			for (auto& coord : cds)
				m_pores.get<tag_hashset>().insert({pores_count, coord});
		}
	}
	++pores_count;
	for (auto& coord : background)
		m_pores.get<tag_hashset>().insert({pores_count, coord});

	after_measure();
}

void MeasureWindow::after_measure()
{
	m_selected_pores.clear();
	m_boundary_pixels.clear();
	m_deleted_pores.clear();
	m_filtered_pores.clear();
	if (Frame::frame->m_statistic->settings_window->m_toggle_background->GetValue())
		m_deleted_pores.insert(pores_count);

	uint32_t color_num = 3*pores_count;
	if (std::size_t size = m_colors.size(); size < color_num)
	{
		m_colors.reserve(color_num);
		for (uint32_t i = size; i < color_num; ++i)
			m_colors.push_back(uint8_t(m_random(m_rand_engine)));
	}
	else if (size > color_num)
		m_colors.erase(m_colors.begin() + color_num, m_colors.end());

	auto pore_it = m_pores.get<tag_multiset>().begin(), pore_end = m_pores.get<tag_multiset>().end();
	for (uint32_t i = pore_it->first;;)
	{
		if (pores_container::index<tag_hashset>::type::iterator end_it = m_pores.get<tag_hashset>().end(), tmp_it;
			pore_it->second.first == 0 || pore_it->second.first == width-1 || pore_it->second.second == 0 || pore_it->second.second == height-1 ||
#define COND(dx, dy) (tmp_it = m_pores.get<tag_hashset>().find(coord_t{pore_it->second.first dx, pore_it->second.second dy})) == end_it || tmp_it->first != i
			COND(, +1) || COND(+1, +1) || COND(+1,) || COND(+1, -1) || COND(, -1) || COND(-1, -1) || COND(-1,) || COND(-1, +1)) [[unlikely]]
			m_boundary_pixels.insert(pore_it->second);
		if (++pore_it == pore_end) [[unlikely]]
			break;
		if (pore_it->first != i) [[unlikely]]
			i = pore_it->first;
	}

	Frame::frame->m_statistic->CollectStatistic();
}