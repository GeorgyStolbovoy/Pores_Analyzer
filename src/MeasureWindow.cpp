#include "MeasureWindow.h"
#include "Frame.h"
#include <forward_list>

#define DECLARE_SLIDER_ID(z, data, s) MeasureWindow::s = wxWindow::NewControlId()
wxWindowID
	MeasureWindow::collapse_morphology_id = wxWindow::NewControlId(3),
	MeasureWindow::collapse_color_id = MeasureWindow::collapse_morphology_id + 1,
	MeasureWindow::collapse_filter_id = MeasureWindow::collapse_morphology_id + 2,
	MeasureWindow::slider_algorithm_id = wxWindow::NewControlId(),
	MeasureWindow::slider_transparency_id = wxWindow::NewControlId(),
	MeasureWindow::slider_test_id = wxWindow::NewControlId(),
	MeasureWindow::slider_thres_id = wxWindow::NewControlId(),
	MeasureWindow::button_color_id = wxWindow::NewControlId(),
	MeasureWindow::button_erosion_id = wxWindow::NewControlId(),
	MeasureWindow::button_dilation_id = wxWindow::NewControlId(),
	MeasureWindow::toggle_color_id = wxWindow::NewControlId(),
	MeasureWindow::toggle_background_id = wxWindow::NewControlId(),
	MeasureWindow::toggle_boundaries_id = wxWindow::NewControlId(),
	BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(DECLARE_SLIDER_ID, ~, SLIDERS_IDS));

wxBEGIN_EVENT_TABLE(MeasureWindow, wxWindow)
    EVT_COMMAND_SCROLL_CHANGED(MeasureWindow::slider_algorithm_id, MeasureWindow::OnChangeDifference)
	EVT_COMMAND_SCROLL_CHANGED(MeasureWindow::slider_transparency_id, MeasureWindow::OnChangeTransparency)
	EVT_COMMAND_SCROLL_CHANGED(MeasureWindow::slider_thres_id, MeasureWindow::OnChangeThreshold)
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
	if (id_to_delete == 0) [[unlikely]]
		return;
	if (event.IsChecked())
	{
		if (m_deleted_pores.insert(id_to_delete).second)
		{
			if (m_selected_pores.contains(id_to_delete)) [[unlikely]]
				Frame::frame->m_image->deselect_pore(id_to_delete);
			Frame::frame->m_statistic->pores_statistic_list->on_pore_deleted(id_to_delete);
		}
		else
			return;
	}
	else
	{
		if (m_deleted_pores.erase(id_to_delete) > 0)
			Frame::frame->m_statistic->pores_statistic_list->on_pore_recovered(id_to_delete);
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
	static bool bug_workaround = true;
	if (bug_workaround)
	{
		if (std::exchange(diff, m_slider_algorithm->GetValue()) == diff)
			bug_workaround = false;
		if (auto view = gil::view(Frame::frame->m_image->image); !view.empty())
		{
			Measure(view.xy_at(0, 0));
			update_image<true>();
		}
	}
	else
		bug_workaround = true;
}

void MeasureWindow::OnChangeTransparency(wxScrollEvent& event)
{
	update_image<false>();
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
			using coords_t = std::unordered_set<coord_t, boost::hash<coord_t>>;

			m_pores.clear();
			references.clear();

			std::forward_list<ImageWindow::Image_t::view_t::iterator> inspecting;
			coords_t coords, background;
			coords_t::iterator find_it, end_it = coords.end();
			auto first_pixel = view.at(0, 0);
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
						first_pixel = view.at(coord_it->first, coord_it->second);
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
									first_pixel = view.at(coord_it->first, coord_it->second);
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
				struct Group;
				using single_pore_t = std::pair<coords_t, float>;
				using maybe_group_t = std::variant<single_pore_t, Group>;
				using pores_list_t = std::forward_list<coords_t>;
				struct Group
				{
					std::forward_list<maybe_group_t> m_elemental_pores;
					coords_t m_erosed;
					uint16_t sep_step; 

					Group(uint16_t sep_step) : sep_step(sep_step) {}
					Group(Group&&) = default;
					Group& operator=(Group&& other) = default;

					std::optional<pores_list_t> filter_and_dilate()
					{
						if (filter(get_max_k()))
						{
							if (auto it = m_elemental_pores.begin(), end = m_elemental_pores.end(); it == end)
								return std::nullopt;
							else if (std::next(it) == end)
								return std::visit([this, &it]<class T>(T&& x) -> std::optional<pores_list_t>
								{
									if constexpr (std::is_same_v<std::decay_t<T>, single_pore_t>)
									{
										m_erosed.merge(std::move(x.first));
										return std::nullopt;
									}
									else
									{
										m_erosed.merge(std::move(x.m_erosed));
										m_elemental_pores.splice_after(it, std::move(x.m_elemental_pores));
										m_elemental_pores.pop_front();
										return dilate();
									}
								}, *it);
						}
						return dilate();
					}

				private:
					float get_max_k()
					{
						float k = 0;
						for (auto& maybe_group : m_elemental_pores)
							std::visit([this, &k]<class T>(T&& x)
							{
								if constexpr (std::is_same_v<std::decay_t<T>, single_pore_t>)
								{
									if (x.second > k)
										k = x.second;
								}
								else if (float subgroup_k = x.get_max_k(); subgroup_k > k)
									k = subgroup_k;
							}, maybe_group);
						return k;
					}

					bool filter(float k)
					{
						bool need_review = false;
						auto prev_it = m_elemental_pores.before_begin(), cur_it = std::next(prev_it), end_it = m_elemental_pores.end();
						do
							std::visit([&, this, k]<class T>(T&& x)
							{
								if constexpr (std::is_same_v<std::decay_t<T>, single_pore_t>)
								{
									if (x.second * 10 < k)
									{
										m_erosed.merge(std::move(x.first));
										cur_it = m_elemental_pores.erase_after(prev_it);
										need_review = true;
									}
									else
									{
										++prev_it;
										++cur_it;
									}
								}
								else
								{
									if (x.filter(k))
									{
										if (auto tmp_it = x.m_elemental_pores.begin(), tmp_end = x.m_elemental_pores.end(); tmp_it == tmp_end)
										{
											if (float replacing_k = x.m_erosed.size()/float(x.sep_step); replacing_k * 10 < k)
											{
												m_erosed.merge(std::move(x.m_erosed));
												need_review = true;
											}
											else
											{
												coords_t replacing_pore{std::move(x.m_erosed)};
												prev_it = m_elemental_pores.emplace_after(prev_it, single_pore_t{std::move(replacing_pore), replacing_k});
											}
											cur_it = m_elemental_pores.erase_after(prev_it);
										}
										else if (std::next(tmp_it) == tmp_end)
											std::visit([&, this]<class V>(V&& v)
											{
												if constexpr (std::is_same_v<std::decay_t<V>, single_pore_t>)
												{
													coords_t replacing_pore{std::move(x.m_erosed)};
													replacing_pore.merge(std::move(v.first));
													prev_it = m_elemental_pores.emplace_after(prev_it, single_pore_t{std::move(replacing_pore), replacing_pore.size()/float(x.sep_step)});
													cur_it = m_elemental_pores.erase_after(prev_it);
												}
												else
												{
													x.m_erosed.merge(std::move(v.m_erosed));
													x.m_elemental_pores.splice_after(tmp_it, std::move(v.m_elemental_pores));
													x.m_elemental_pores.pop_front();
													++prev_it;
													++cur_it;
												}
											}, *tmp_it);
									}
									else
									{
										++prev_it;
										++cur_it;
									}
								}
							}, *cur_it);
						while (cur_it != end_it);
						return need_review;
					}

					pores_list_t dilate()
					{
						pores_list_t ret_list, from_subgroups_list;

						for (auto& elem_pore : m_elemental_pores)
						{
							std::visit([&, this]<class T>(T&& x)
							{
								if constexpr (std::is_same_v<std::decay_t<T>, single_pore_t>)
									ret_list.push_front(std::move(x.first));
								else
									from_subgroups_list.splice_after(from_subgroups_list.before_begin(), x.dilate());
							}, elem_pore);
						}

						if (!ret_list.empty())
							do
							{
								std::unordered_multimap<coords_t*, coords_t::iterator> iters_to_dilate;
								auto erosed_it = m_erosed.begin(), erosed_end = m_erosed.end();
								do
								{
									for (auto elem_pores_it = ret_list.begin(), elem_pores_end = ret_list.end(), elem_pores_owner = elem_pores_end;;)
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

						ret_list.splice_after(ret_list.before_begin(), std::move(from_subgroups_list));
						return ret_list;
					}
				};

				static std::optional<Group> separate_to_group(coords_t& coords, uint16_t sep_step = 1)
				{
					// Debug
					if (coords.size() == 16684)
					{
						Sleep(0);
					}

					Group group{sep_step};

					for (;;)
					{
						++sep_step;
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
								group.m_elemental_pores.emplace_front(single_pore_t{std::move(elemental_pore), elemental_pore.size() / float(sep_step)});
								auto elem_pores_end = group.m_elemental_pores.end(), prev_it = group.m_elemental_pores.before_begin(), next_it = std::next(prev_it);
#define ADVANCE_SEPARATION(chk) \
									if (auto opt_separated_pores = separate_to_group(std::get<single_pore_t>(*next_it).first, sep_step); opt_separated_pores.has_value()) \
									{ \
										++next_it; \
										group.m_elemental_pores.erase_after(prev_it); \
										prev_it = group.m_elemental_pores.insert_after(prev_it, std::move(opt_separated_pores.value())); \
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
							group.m_elemental_pores.emplace_front(single_pore_t{std::move(elemental_pore), elemental_pore.size()/float(sep_step)});
						}
					}
				}

			public:
				static std::optional<pores_list_t> separate(coords_t& coords)
				{
					if (auto opt_separated_pores = separate_to_group(coords); opt_separated_pores.has_value())
					{
						if (auto opt_true_pores = opt_separated_pores.value().filter_and_dilate(); opt_true_pores.has_value())
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
			auto image_view = gil::view(Frame::frame->m_image->image);
			for (coords_t& cds : pores)
			{
				if (auto opt_separated_pores = Separation::separate(cds); opt_separated_pores.has_value())
				{
					auto sep_pores_it = opt_separated_pores.value().begin(), sep_pores_end = opt_separated_pores.value().end();
					do
					{
						++pores_count;
						float reference = 0.0f;
						for (auto& coord : *sep_pores_it)
						{
							m_pores.get<tag_multiset>().insert({pores_count, coord});
							reference += *image_view.xy_at(coord.first, coord.second);
						}
						references[pores_count] = reference / sep_pores_it->size();
					} while (++sep_pores_it != sep_pores_end);
				}
				else
				{
					++pores_count;
					float reference = 0.0f;
					for (auto& coord : cds)
					{
						m_pores.get<tag_multiset>().insert({pores_count, coord});
						reference += *image_view.xy_at(coord.first, coord.second);
					}
					references[pores_count] = reference / cds.size();
				}
			}
			++pores_count;
			float reference = 0.0f;
			for (auto& coord : background)
			{
				m_pores.get<tag_multiset>().insert({pores_count, coord});
				reference += *image_view.xy_at(coord.first, coord.second);
			}
			references[pores_count] = reference / background.size();

			after_measure();
			update_image<true>();
		}
	}
	else
		bug_workaround = true;
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
	Frame::frame->Layout();
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
				if (!self->m_deleted_pores.contains(pore_id) && !self->m_filtered_pores.contains(pore_id))
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
				if (self->m_deleted_pores.contains(old_id) || self->m_filtered_pores.contains(old_id)) [[unlikely]]
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
			if (auto result = m_pores.get<tag_hashset>().insert(pore); !result.second && (m_deleted_pores.contains(result.first->first) || m_filtered_pores.contains(result.first->first))) [[unlikely]]
				m_pores.get<tag_multiset>().modify_key(m_pores.project<tag_multiset>(result.first), [&pore](uint32_t& id){id = pore.first;});
		}
		after_measure();
		update_image<true>();
	};

	for (auto it = m_pores.get<tag_multiset>().begin(), end = m_pores.get<tag_multiset>().end();;)
	{
		uint32_t pore_id = it->first;
		if (!m_deleted_pores.contains(pore_id) && !m_filtered_pores.contains(pore_id))
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
		else if ((it = m_pores.get<tag_multiset>().lower_bound(pore_id, [](uint32_t lhs, uint32_t rhs) {return lhs <= rhs;}))
			== m_pores.get<tag_multiset>().end()) [[unlikely]]
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
	test = m_slider_test->GetValue();

	pores_count = 0;
	m_pores.clear();
	references.clear();

	inspect_pore({std::move(loc), {0, 0}});
	for (auto chk_it = m_checklist.get<tag_sequenced>().begin(); chk_it != m_checklist.get<tag_sequenced>().end(); ++chk_it)
	{
		if (!m_pores.get<tag_hashset>().contains(chk_it->second)) [[unlikely]]
			inspect_pore(*chk_it);
	}
	m_checklist.clear();
	references.clear();

	// Фон запомнить, но не обрабатывать
	auto old_pores_it_1 = m_pores.get<tag_multiset>().begin(), old_pores_it_end = m_pores.get<tag_multiset>().end();
	uint32_t max_id = old_pores_it_1->first;
	auto old_pores_it_2 = m_pores.get<tag_multiset>().upper_bound(max_id);
	for (uint32_t old_id = max_id, square = std::distance(old_pores_it_1, old_pores_it_2);;)
	{
		old_pores_it_1 = old_pores_it_2;
		if (old_pores_it_1 == old_pores_it_end) [[unlikely]]
			break;
		old_id = old_pores_it_1->first;
		old_pores_it_2 = m_pores.get<tag_multiset>().upper_bound(old_id);
		if (uint32_t tmp_square = std::distance(old_pores_it_1, old_pores_it_2); tmp_square > square) [[unlikely]]
		{
			max_id = old_id;
			square = tmp_square;
		}
	}
	
	// 1) Некоторые поры поглотили часть других, и те могут быть разъединены -> проверить цельность, для разорванных пересчитать
	// среднее значение и, может быть, объединить с соседом
	// 2) Некоторые поры поглотили другие целиком -> пересчитать все идентификаторы для восстановления порядка
	// 3) Объединить вложенные "поры"
	{
#define SKIP_BACKGROUND(code) \
			if (old_pores_it_1->first == max_id) [[unlikely]] \
			{ \
				old_pores_it_1 = m_pores.get<tag_multiset>().upper_bound(max_id); \
				if (old_pores_it_1 == old_pores_it_end) [[unlikely]] \
					code \
			}

		pores_count = 1;
		pores_container separated_pores;
		old_pores_it_1 = m_pores.get<tag_multiset>().begin();
		SKIP_BACKGROUND({
			after_measure();
			return;
		})
		separated_pores.insert(m_pores.get<tag_multiset>().extract(old_pores_it_1));
		auto image_view = gil::view(Frame::frame->m_image->image);
		auto pores_it = separated_pores.get<tag_multiset>().begin(), pores_end = separated_pores.get<tag_multiset>().end();
		auto loc_picker = image_view.xy_at(pores_it->second.first, pores_it->second.second);
		float reference = *loc_picker;
		uint32_t pore_pix_count = 1;
		for (uint32_t old_id = pores_it->first;;)
		{
#define INSPECT(r, d, ofs) \
				if (auto find_it = m_pores.get<tag_hashset>().find(coord_t{pores_it->second.first + BOOST_PP_TUPLE_ELEM(0, ofs), pores_it->second.second + BOOST_PP_TUPLE_ELEM(1, ofs)}); \
					find_it != m_pores.get<tag_hashset>().end() && find_it->first != max_id) \
				{ \
					++pore_pix_count; \
					reference += loc_picker[BOOST_PP_CAT(cl_, BOOST_PP_TUPLE_ELEM(2, ofs))]; \
					auto node = m_pores.get<tag_hashset>().extract(find_it); \
					node.value().first = pores_count; \
					separated_pores.get<tag_multiset>().insert(std::move(node)); \
				}
			BOOST_PP_SEQ_FOR_EACH(INSPECT, ~, ((0, 1, b))((1, 1, rb))((1, 0, r))((1, -1, rt))((0, -1, t))((-1, -1, lt))((-1, 0, l))((-1, 1, lb)))
#undef INSPECT
			if (++pores_it == pores_end) [[unlikely]]
			{
				references[pores_count] = reference / pore_pix_count;
				old_pores_it_1 = m_pores.get<tag_multiset>().find(old_id);
				if (old_pores_it_1 == old_pores_it_end)
				{
					old_pores_it_1 = m_pores.get<tag_multiset>().upper_bound(old_id);
					if (old_pores_it_1 == old_pores_it_end) [[unlikely]]
						break;
					SKIP_BACKGROUND(break;)
					old_id = old_pores_it_1->first;
				}
				auto node = m_pores.get<tag_multiset>().extract(old_pores_it_1);
				node.value().first = ++pores_count;
				pores_it = separated_pores.insert(std::move(node)).position;
				pore_pix_count = 1;
				loc_picker = image_view.xy_at(pores_it->second.first, pores_it->second.second);
				reference = *loc_picker;
			}
			else
				loc_picker = image_view.xy_at(pores_it->second.first, pores_it->second.second);
		}
		old_pores_it_1 = m_pores.get<tag_multiset>().begin();
		reference = *image_view.at(old_pores_it_1->second.first, old_pores_it_1->second.second);
		auto node = m_pores.get<tag_multiset>().extract(old_pores_it_1++);
		node.value().first = ++pores_count;
		for (pore_pix_count = 1;; ++pore_pix_count)
		{
			separated_pores.insert(std::move(node));
			if (old_pores_it_1 == old_pores_it_end) [[unlikely]]
				break;
			reference += *image_view.at(old_pores_it_1->second.first, old_pores_it_1->second.second);
			node = m_pores.get<tag_multiset>().extract(old_pores_it_1++);
			node.value().first = pores_count;
		}
		references[pores_count] = reference / pore_pix_count;
		separated_pores.swap(m_pores);
	}

	after_measure();
}

void MeasureWindow::inspect_pore(const inspecting_pixel& insp_pixel)
{
	inspecting_pixels_t inspecting{{insp_pixel}};
	std::unordered_set<
		inspecting_pixel,
		decltype([](const auto& pore){return boost::hash<coord_t>{}(pore.second);}),
		decltype([](const auto& lhs, const auto& rhs){return lhs.second.first == rhs.second.first && lhs.second.second == rhs.second.second;})
	> local_checklist, other_pixels;
	uint32_t pore_num = ++pores_count, count = 0, sum = 0;
	float& reference = references.emplace(pore_num, *insp_pixel.first).first->second;
	m_pores.get<tag_multiset>().insert({pore_num, insp_pixel.second});

	for (auto insp_it = inspecting.get<tag_sequenced>().begin(), insp_end = inspecting.get<tag_sequenced>().end();;)
	{
		do
		{
			sum += *insp_it->first;
			++count;
			std::ptrdiff_t x = insp_it->second.first, y = insp_it->second.second;
			bool under_top = y > 0, above_bottom = y < height-1, before_left = x > 0, before_right = x < width-1;

#define INSPECT(cond, dx, dy, dir, opt_2nd_inspect) \
			if (cond) \
			{ \
				{ \
					coord_t coord{x + dx, y + dy}; \
					if (auto find_it = m_pores.get<tag_hashset>().find(coord); find_it == m_pores.get<tag_hashset>().end()) \
					{ \
						if (std::abs(reference - insp_it->first[cl_##dir]) <= diff) \
						{ \
							inspecting.get<tag_hashset>().insert({insp_it->first + gil::point(dx, dy), coord}); \
							m_pores.get<tag_hashset>().insert({pore_num, coord}); \
						} \
						else \
							local_checklist.insert({insp_it->first + gil::point(dx, dy), coord}); \
					} \
					else if (find_it->first != pore_num) \
						other_pixels.insert({insp_it->first + gil::point(dx, dy), coord}); \
				} \
				opt_2nd_inspect \
			}
			INSPECT(above_bottom, 0, 1, b, INSPECT(before_right, 1, 1, rb,))
			INSPECT(before_right, 1, 0, r, INSPECT(under_top, 1, -1, rt,))
			INSPECT(under_top, 0, -1, t, INSPECT(before_left, -1, -1, lt,))
			INSPECT(before_left, -1, 0, l, INSPECT(above_bottom, -1, 1, lb,))
#undef INSPECT
		} while(++insp_it != insp_end);
		reference = sum / float(count);
		auto saved_pos_it = insp_end;
		--saved_pos_it;
		for (auto chk_it = local_checklist.begin(), chk_end = local_checklist.end(); chk_it != chk_end;)
		{
			if (std::abs(reference - *chk_it->first) <= diff) [[unlikely]]
			{
				inspecting.get<tag_hashset>().insert(*chk_it);
				m_pores.get<tag_multiset>().insert({pore_num, chk_it->second});
				local_checklist.erase(chk_it++);
			}
			else
				++chk_it;
		}
		for (auto oth_it = other_pixels.begin(), oth_end = other_pixels.end(); oth_it != oth_end;)
		{
			if (auto find_it = m_pores.get<tag_hashset>().find(oth_it->second);
				[](float a, float b, int v){return a + b > 0 ? 100 * a/(a + b) <= v : false;}(std::abs(reference - *oth_it->first), std::abs(references[find_it->first] - *oth_it->first), test)) [[unlikely]]
			{
				inspecting.get<tag_hashset>().insert(*oth_it);
				m_pores.get<tag_multiset>().modify_key(m_pores.project<tag_multiset>(find_it), [pore_num](uint32_t& id){id = pore_num;});
				other_pixels.erase(oth_it++);
			}
			else
				++oth_it;
		}
		if (++saved_pos_it != insp_end)
			insp_it = saved_pos_it;
		else
			break;
	}
	for (auto& p : local_checklist)
		m_checklist.get<tag_hashset>().insert(std::move(p));
}

// Игнорирует отфильтрованные поры
uint32_t MeasureWindow::get_biggest_pore_id()
{
	uint32_t id_to_delete = 0;
	for (uint32_t i = 1, max_count = 0; i <= pores_count; ++i)
		if (!m_filtered_pores.contains(i))
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
	ImageWindow* iw = Frame::frame->m_image;
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
	m_filtered_pores.clear();
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