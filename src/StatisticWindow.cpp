#include "StatisticWindow.h"
#include "Frame.h"
#include "Utils.h"
#include <wx/dcbuffer.h>
#include <wx/statbox.h>
#include <charconv>
#include <boost/preprocessor.hpp>

struct pore_statistic_collector
{
	using multiset_iterator_t = MeasureWindow::pores_container::index<MeasureWindow::tag_multiset>::type::iterator;
	using hashset_iterator_t = MeasureWindow::pores_container::index<MeasureWindow::tag_hashset>::type::iterator;

	MeasureWindow* measure;
	StatisticWindow::pores_statistic_container& container;
	multiset_iterator_t it, end;
	hashset_iterator_t tmp_it, end_it;
	uint32_t square, perimeter, sum_x, sum_y, min_x, max_x, min_y, max_y, lenght_oy, width_ox;
	float shape, diameter, elongation;
	std::pair<float, float> centroid;

	pore_statistic_collector(MeasureWindow* measure, StatisticWindow::pores_statistic_container& container)
		: measure(measure), container(container), end(measure->m_pores.get<MeasureWindow::tag_multiset>().end()), end_it(measure->m_pores.get<MeasureWindow::tag_hashset>().end())
	{}
	void collect(uint32_t id)
	{
		square = 0; perimeter = 0; sum_x = 0; sum_y = 0; min_x = measure->width - 1; max_x = 0; min_y = measure->height - 1; max_y = 0;
		do
		{
			++square;
			sum_x += it->second.first;
			sum_y += it->second.second;
			if (min_x > uint32_t(it->second.first)) [[unlikely]] min_x = it->second.first;
			if (max_x < uint32_t(it->second.first)) [[unlikely]] max_x = it->second.first;
			if (min_y > uint32_t(it->second.second)) [[unlikely]] min_y = it->second.second;
			if (max_y < uint32_t(it->second.second)) [[unlikely]] max_y = it->second.second;
			if (measure->m_boundary_pixels.contains(it->second)) [[unlikely]]
			{
#define INC_IF_BOUNDARY(dx, dy) if (auto tmp_it = measure->m_pores.get<MeasureWindow::tag_hashset>().find(MeasureWindow::coord_t{it->second.first dx, it->second.second dy}); tmp_it == end_it || tmp_it->first != id) ++perimeter
				INC_IF_BOUNDARY(-1,);
				INC_IF_BOUNDARY(,+1);
				INC_IF_BOUNDARY(+1,);
				INC_IF_BOUNDARY(,-1);
			}
		} while (++it != end && it->first == id);
		lenght_oy = max_y - min_y + 1;
		width_ox = max_x - min_x + 1;
		shape = 4*M_PI*square/(perimeter*perimeter);
		diameter = std::sqrtf(4*square/M_PI);
		elongation = 1/shape;
		centroid.first = sum_x/float(square);
		centroid.second = sum_y/float(square);
		container.emplace_back(id, square, perimeter, diameter, centroid, 0, 0, lenght_oy, width_ox, 0, 0, shape, elongation);
	}
};

// StatisticWindow

wxBEGIN_EVENT_TABLE(StatisticWindow, wxWindow)
	EVT_PAINT(StatisticWindow::OnPaint)
wxEND_EVENT_TABLE()

StatisticWindow::StatisticWindow(wxWindow* parent)
	: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN), parent_frame(static_cast<Frame*>(parent))
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	
	m_aui = new Aui{this};

	common_statistic_list = new CommonStatisticList(this);
	common_statistic_list ->AppendColumn(wxT("Параметр"));
	common_statistic_list ->AppendColumn(wxT("Значение"));
	m_aui->AddPane(common_statistic_list, wxAuiPaneInfo{}.Caption(wxT("Общая характеристика")).CloseButton(false).Top().MaximizeButton().PaneBorder(true).Name(wxT("Common")));

	pores_statistic_list = new PoresStatisticList(this);
	pores_statistic_list->AppendColumn(wxT("№"));
	pores_statistic_list->AppendColumn(wxT("Площадь"));
	pores_statistic_list->AppendColumn(wxT("Периметр"));
	pores_statistic_list->AppendColumn(wxT("Эквивалентный диаметр"));
	pores_statistic_list->AppendColumn(wxT("Центроид"));
	pores_statistic_list->AppendColumn(wxT("Длина"));
	pores_statistic_list->AppendColumn(wxT("Ширина"));
	pores_statistic_list->AppendColumn(wxT("Высота проекции"));
	pores_statistic_list->AppendColumn(wxT("Ширина проекции"));
	pores_statistic_list->AppendColumn(wxT("Наибольший диаметр"));
	pores_statistic_list->AppendColumn(wxT("Наименьший диаметр"));
	pores_statistic_list->AppendColumn(wxT("Фактор формы"));
	pores_statistic_list->AppendColumn(wxT("Удлинённость"));
	m_aui->AddPane(pores_statistic_list, wxAuiPaneInfo{}.Caption(wxT("Характеристики пор")).CloseButton(false).Top().MaximizeButton().PaneBorder(true).Name(wxT("Pores")));

	settings_window = new SettingsWindow(this);
	m_aui->AddPane(settings_window, wxAuiPaneInfo{}.CenterPane().Name(wxT("Settings")));

	distribution_window = new DistributionWindow(this);
	m_aui->AddPane(distribution_window, wxAuiPaneInfo{}.Caption(wxT("Распределение")).CloseButton(false).Right().MaximizeButton().PaneBorder(true).Name(wxT("Distrib")));

	common_statistic_list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
	common_statistic_list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
	for (uint8_t i = 0, count = pores_statistic_list->GetColumnCount(); i < count; ++i)
		pores_statistic_list->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);

	m_aui->Update();
}

void StatisticWindow::CollectStatistic()
{
	MeasureWindow* measure = parent_frame->m_measure;
	bool collect_deleted = settings_window->m_checkBox_deleted->IsChecked();
	uint32_t pores_num = measure->pores_count - measure->m_deleted_pores.size(), pores_square = 0;
	double perimeter_mean = 0, diameter_mean = 0, sum_x_mean = 0, sum_y_mean = 0, lenght_oy_mean = 0, width_ox_mean = 0, shape_mean = 0, elongation_mean = 0;

	common_statistic_list->SetItemCount(6);
	pores_statistic_list->attributes.clear();
	pores_statistic_list->container.clear();
	pores_statistic_list->container.reserve((collect_deleted ? measure->pores_count : pores_num) + 4);
	pores_statistic_list->container.resize(2, {0, 0, 0, 0, {0, 0}, 0, 0, 0, 0, 0, 0, 0, 0});
	constexpr float fmax = std::numeric_limits<float>::max();
	pores_statistic_list->container.push_back({0, fmax, fmax, fmax, {fmax, fmax}, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax});
	pores_statistic_list->container.push_back({0, 0, 0, 0, {0, 0}, 0, 0, 0, 0, 0, 0, 0, 0});
	pores_statistic_list->SetItemCount((collect_deleted ? measure->pores_count : pores_num) + 4);

	pore_statistic_collector psc(measure, pores_statistic_list->container);
	psc.it = measure->m_pores.get<MeasureWindow::tag_multiset>().begin();
	do
	{
		uint32_t id = psc.it->first;
		if (bool not_deleted = !measure->m_deleted_pores.contains(id); not_deleted || collect_deleted)
		{
			psc.collect(id);
			if (not_deleted)
			{
				pores_square += psc.square;
				perimeter_mean += psc.perimeter;
				diameter_mean += psc.diameter;
				sum_x_mean += psc.centroid.first;
				sum_y_mean += psc.centroid.second;
				lenght_oy_mean += psc.lenght_oy;
				width_ox_mean += psc.width_ox;
				shape_mean += psc.shape;
				elongation_mean += psc.elongation;

#define PARAMS_VAARGS (psc.square),(psc.perimeter),(psc.diameter),(psc.centroid.first, psc.centroid.second),(psc.lenght_oy),(psc.width_ox),(psc.shape),(psc.elongation)
#define PARAMS BOOST_PP_VARIADIC_TO_SEQ(PARAMS_VAARGS)
#define SET_MIN_MAX_IMPL(value, param, op) \
				if (value op param) [[unlikely]] \
					value = param;
#define SET_MIN_MAX(z, n, flag) \
				{ \
					auto& value = std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(pores_statistic_list->container[flag]); \
					BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), \
						SET_MIN_MAX_IMPL(value, BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(n, PARAMS)), BOOST_PP_IF(BOOST_PP_EQUAL(flag, 2), >, <)), \
						SET_MIN_MAX_IMPL(value.first, BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(n, PARAMS)), BOOST_PP_IF(BOOST_PP_EQUAL(flag, 2), >, <)) \
						SET_MIN_MAX_IMPL(value.second, BOOST_PP_TUPLE_ELEM(1, BOOST_PP_SEQ_ELEM(n, PARAMS)), BOOST_PP_IF(BOOST_PP_EQUAL(flag, 2), >, <))) \
				}
				BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(PARAMS), SET_MIN_MAX, 2)
				BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(PARAMS), SET_MIN_MAX, 3)
#undef PARAMS_VAARGS
#undef PARAMS
			}
			else
			{
				wxItemAttr attr;
				attr.SetBackgroundColour(0xAAAAEE);
				pores_statistic_list->attributes.insert({id, attr});
			}
		}
		else
			psc.it = measure->m_pores.get<MeasureWindow::tag_multiset>().lower_bound(id, [](uint32_t lhs, uint32_t rhs) {return lhs <= rhs;});
	} while (psc.it != psc.end);

	uint32_t all_size = measure->width*measure->height;
	common_statistic_list->container[0] = CommonStatisticList::row_t{wxString{"Ширина"}, measure->width};
	common_statistic_list->container[1] = CommonStatisticList::row_t{wxString{"Высота"}, measure->height};
	common_statistic_list->container[2] = CommonStatisticList::row_t{wxString{"Количество пор"}, pores_num};
	common_statistic_list->container[3] = CommonStatisticList::row_t{wxString{"Удельное количество пор"}, pores_square/float(all_size)};
	common_statistic_list->container[4] = CommonStatisticList::row_t{wxString{"Площадь пор"}, pores_square};
	common_statistic_list->container[5] = CommonStatisticList::row_t{wxString{"Площадь матрицы"}, all_size - pores_square};

#define PARAMS_VAARGS (pores_square),(perimeter_mean),(diameter_mean),(sum_x_mean, sum_y_mean),(lenght_oy_mean),(width_ox_mean),(shape_mean),(elongation_mean)
#define PARAMS BOOST_PP_VARIADIC_TO_SEQ(PARAMS_VAARGS)
#define SET_MEAN_DEVIATION(z, n, __) \
	{ \
		auto& mean = std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(pores_statistic_list->container[0]); \
		BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), \
			mean = BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(n, PARAMS))/float(pores_num);, \
			mean.first = BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(n, PARAMS))/pores_num; \
			mean.second = BOOST_PP_TUPLE_ELEM(1, BOOST_PP_SEQ_ELEM(n, PARAMS))/pores_num;) \
		BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), double deviation = 0;, double deviation_x = 0; double deviation_y = 0;) \
		for (auto it = pores_statistic_list->container.begin() + 4, end = pores_statistic_list->container.end(); it != end; ++it) \
			if (!pores_statistic_list->attributes.contains(std::get<PoresStatisticList::ID>(*it))) \
			BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), \
				deviation += std::pow(mean - std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it), 2);, \
				{ \
					deviation_x += std::pow(mean.first - std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it).first, 2); \
					deviation_y += std::pow(mean.second - std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it).second, 2); \
				}) \
		std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(pores_statistic_list->container[1]) = \
			BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), std::sqrtf(deviation/(pores_num-1));, (std::pair{std::sqrtf(deviation_x/(pores_num-1)), std::sqrtf(deviation_y/(pores_num-1))});) \
	}
	BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(PARAMS), SET_MEAN_DEVIATION, ~)
#undef PARAMS_VAARGS
#undef PARAMS

	common_statistic_list->SetColumnWidth(0, wxLIST_AUTOSIZE);
	common_statistic_list->SetColumnWidth(1, wxLIST_AUTOSIZE);
	pores_statistic_list->set_columns_width();
	common_statistic_list->Refresh();
	pores_statistic_list->Refresh();
	common_statistic_list->Update();
	pores_statistic_list->Update();
}

void StatisticWindow::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);
	dc.Clear();
}

// Aui

void StatisticWindow::Aui::InitPanesPositions(wxSize size)
{
	int sashSize = m_art->GetMetric(wxAUI_DOCKART_SASH_SIZE);
	for (uint8_t i = 0, dock_count = m_docks.GetCount(); i < dock_count; ++i)
	{
		wxAuiDockInfo& d = m_docks.Item(i);
		if (d.dock_direction == wxAUI_DOCK_TOP)
			d.size = size.GetHeight()/2;
		else if (d.dock_direction == wxAUI_DOCK_RIGHT)
			d.size = size.GetWidth()/2;
		else
			continue;
		d.size -= sashSize;
	}
	Update();
}

// CommonStatisticList

StatisticWindow::CommonStatisticList::CommonStatisticList(wxWindow* parent)
	: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL | wxLC_HRULES | wxLC_VRULES)
{
	EnableAlternateRowColours();
}

wxString StatisticWindow::CommonStatisticList::OnGetItemText(long item, long column) const
{
	char buf[16];
#define CASE_NUMBER(value) case value: *std::to_chars(buf, buf + 16, std::get<value>(container[item])).ptr = '\0'; return {buf};
	switch (column)
	{
		case PARAM_NAME: return std::get<PARAM_NAME>(container[item]);
		case PARAM_VALUE: *std::to_chars(buf, buf + 16, std::get<PARAM_VALUE>(container[item])).ptr = '\0'; return {buf};
	}
	__assume(false);
}

// PoresStatisticList

wxBEGIN_EVENT_TABLE(StatisticWindow::PoresStatisticList, wxWindow)
	EVT_LIST_ITEM_SELECTED(wxID_ANY, StatisticWindow::PoresStatisticList::OnSelection)
	EVT_LIST_ITEM_DESELECTED(wxID_ANY, StatisticWindow::PoresStatisticList::OnDeselection)
wxEND_EVENT_TABLE()

StatisticWindow::PoresStatisticList::PoresStatisticList(StatisticWindow* parent)
	: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL | wxLC_HRULES | wxLC_VRULES), parent_statwindow(parent)
{
	EnableAlternateRowColours();
}

wxString StatisticWindow::PoresStatisticList::OnGetItemText(long item, long column) const
{
	char buf[16];
#define CASE_NUMBER(value) case value: *std::to_chars(buf, buf + 16, std::get<value>(container[item])).ptr = '\0'; return {buf};

	switch (column)
	{
	case ID:
	{
		switch (item)
		{
		[[unlikely]] case 0: return {wxT("Среднее значение")};
		[[unlikely]] case 1: return {wxT("Стандартное отклонение")};
		[[unlikely]] case 2: return {wxT("Минимальное значение")};
		[[unlikely]] case 3: return {wxT("Максимальное значение")};
		[[likely]] default:  *std::to_chars(buf, buf + 16, std::get<ID>(container[item])).ptr = '\0'; return {buf};
		}
	}
	CASE_NUMBER(SQUARE)
	CASE_NUMBER(PERIMETER)
	CASE_NUMBER(DIAMETER)
	case CENTROID:
	{
		wxString result{wxT("(")};
		auto [x, y] = std::get<CENTROID>(container[item]);
		*std::to_chars(buf, buf + 16, x).ptr = '\0';
		(result.Append(buf)).Append(wxT("; "));
		*std::to_chars(buf, buf + 16, y).ptr = '\0';
		return (result.Append(buf)).Append(wxT(")"));
	}
	CASE_NUMBER(LENGTH)
	CASE_NUMBER(WIDTH)
	CASE_NUMBER(LENGTH_OY)
	CASE_NUMBER(WIDTH_OX)
	CASE_NUMBER(MAX_DIAMETER)
	CASE_NUMBER(MIN_DIAMETER)
	CASE_NUMBER(SHAPE)
	CASE_NUMBER(ELONGATION)
	}
	__assume(false);
}

wxItemAttr* StatisticWindow::PoresStatisticList::OnGetItemAttr(long item) const
{
	if (item < 4) [[unlikely]]
	{
		struct static_attribute {wxItemAttr item; static_attribute() {item.SetBackgroundColour(0xFFEEEE);}};
		static static_attribute _item;
		return &_item.item;
	}
	else [[likely]] if (auto find_it = attributes.find(item-3); find_it != attributes.end()) [[unlikely]]
		return const_cast<wxItemAttr*>(&find_it->second);
	else [[likely]]
		return wxListCtrl::OnGetItemAttr(item);
}

void StatisticWindow::PoresStatisticList::OnSelection(wxListEvent& event)
{
	if (dont_affect)
		return;
	long item_index = event.GetIndex(), pore_id = item_index - 3;
	if (item_index > 3)
	{
		if (!parent_statwindow->settings_window->m_checkBox_deleted->IsChecked())
			pore_id += std::distance(parent_statwindow->parent_frame->m_measure->m_deleted_pores.begin(), 
				parent_statwindow->parent_frame->m_measure->m_deleted_pores.lower_bound(std::get<ID>(container[item_index])));
		if (!parent_statwindow->parent_frame->m_measure->m_deleted_pores.contains(pore_id))
		{
			if (!parent_statwindow->parent_frame->m_image->m_sel_session.has_value())
				parent_statwindow->parent_frame->m_image->m_sel_session.emplace(parent_statwindow->parent_frame->m_image);
			parent_statwindow->parent_frame->m_image->m_sel_session.value().select(parent_statwindow->parent_frame->m_measure, pore_id);
			auto [pore_x, pore_y] = std::get<CENTROID>(container[item_index]);
			parent_statwindow->parent_frame->m_image->scale_center.x = nearest_integer<int>(pore_x);
			parent_statwindow->parent_frame->m_image->scale_center.y = nearest_integer<int>(pore_y);
			parent_statwindow->parent_frame->m_image->Refresh();
			parent_statwindow->parent_frame->m_image->Update();
		}
	}
}

void StatisticWindow::PoresStatisticList::OnDeselection(wxListEvent& event)
{
	if (dont_affect)
		return;
	std::vector<long> selected;
	selected.reserve(GetSelectedItemCount());
	if (parent_statwindow->settings_window->m_checkBox_deleted->IsChecked())
		for (long item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); item != -1; item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED))
			selected.push_back(item-3);
	else
		for (long item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); item != -1; item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED))
			selected.push_back(item - 3 + std::distance(parent_statwindow->parent_frame->m_measure->m_deleted_pores.begin(), 
				parent_statwindow->parent_frame->m_measure->m_deleted_pores.lower_bound(std::get<ID>(container[item]))));
	bool do_refresh = false;
	for (auto it = parent_statwindow->parent_frame->m_measure->m_selected_pores.begin(), end = parent_statwindow->parent_frame->m_measure->m_selected_pores.end(); it != end;)
	{
		uint32_t id = *it++;
		if (auto find_it = std::lower_bound(selected.begin(), selected.end(), long(id)); find_it == selected.end() || *find_it != id)
		{
			parent_statwindow->parent_frame->m_image->m_sel_session.value().deselect(parent_statwindow->parent_frame->m_measure, id);
			do_refresh = true;
		}
	}
	if (do_refresh)
	{
		if (parent_statwindow->parent_frame->m_measure->m_selected_pores.empty())
			parent_statwindow->parent_frame->m_image->m_sel_session = std::nullopt;
		parent_statwindow->parent_frame->m_image->Refresh();
		parent_statwindow->parent_frame->m_image->Update();
	}
}

void StatisticWindow::PoresStatisticList::select_item(uint32_t pore_id)
{
	dont_affect = true;
	long item_index = pore_id + (parent_statwindow->settings_window->m_checkBox_deleted->IsChecked() ? 3 : 3 - std::distance(parent_statwindow->parent_frame->m_measure->m_deleted_pores.begin(), 
				parent_statwindow->parent_frame->m_measure->m_deleted_pores.lower_bound(pore_id)));
	SetItemState(item_index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	wxRect rect;
	GetItemRect(0, rect);
	ScrollList(0, rect.height*(item_index - GetScrollPos(wxVERTICAL)));
	dont_affect = false;
}

void StatisticWindow::PoresStatisticList::deselect_item(uint32_t pore_id)
{
	dont_affect = true;
	long item_index = pore_id + (parent_statwindow->settings_window->m_checkBox_deleted->IsChecked() ? 3 : 3 - std::distance(parent_statwindow->parent_frame->m_measure->m_deleted_pores.begin(), 
		parent_statwindow->parent_frame->m_measure->m_deleted_pores.lower_bound(pore_id)));
	SetItemState(item_index, 0, wxLIST_STATE_SELECTED);
	dont_affect = false;
}

void StatisticWindow::PoresStatisticList::deselect_all()
{
	dont_affect = true;
	for (long item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); item != -1; item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED))
		if (item > 3)
			SetItemState(item, 0, wxLIST_STATE_SELECTED);
	dont_affect = false;
}

void StatisticWindow::PoresStatisticList::on_pore_deleted(uint32_t pore_id)
{
	MeasureWindow* measure = parent_statwindow->parent_frame->m_measure;
	long item_index = pore_id + 3;
	if (parent_statwindow->settings_window->m_checkBox_deleted->IsChecked())
	{
		wxItemAttr attr;
		attr.SetBackgroundColour(0xAAAAEE);
		attributes.insert({pore_id, attr});
	}
	else
		item_index -= std::distance(parent_statwindow->parent_frame->m_measure->m_deleted_pores.begin(), parent_statwindow->parent_frame->m_measure->m_deleted_pores.lower_bound(pore_id));;
	auto item_iter = container.begin() + item_index;
	row_t item_row = *item_iter;
	if (!parent_statwindow->settings_window->m_checkBox_deleted->IsChecked())
		container.erase(item_iter);

	auto recalculate = [this, &item_row]<uint8_t param_name, bool is_min>()
	{
#pragma warning(disable : 5103)
#define CHECK_MIN_MAX(...) \
			if (conditional<is_min>(std::get<param_name>(item_row)##__VA_OPT__(.)__VA_ARGS__ <= value##__VA_OPT__(.)__VA_ARGS__, \
				std::get<param_name>(item_row)##__VA_OPT__(.)__VA_ARGS__ >= value##__VA_OPT__(.)__VA_ARGS__)) [[unlikely]] \
			{ \
				value##__VA_OPT__(.)__VA_ARGS__ = conditional<is_min>(std::numeric_limits<float>::max(), std::numeric_limits<float>::min()); \
				for (auto it = container.begin() + 4, end = container.end(); it != end; ++it) \
					if (!attributes.contains(std::get<ID>(*it))) \
						if (auto tmp_value = std::get<param_name>(*it); \
							conditional<is_min>(tmp_value##__VA_OPT__(.)__VA_ARGS__ < value##__VA_OPT__(.)__VA_ARGS__, tmp_value##__VA_OPT__(.)__VA_ARGS__ > value##__VA_OPT__(.)__VA_ARGS__)) [[unlikely]] \
							value##__VA_OPT__(.)__VA_ARGS__ = tmp_value##__VA_OPT__(.)__VA_ARGS__; \
			}

		auto& value = std::get<param_name>(container[conditional<is_min>(2, 3)]);
		using T = std::remove_reference_t<decltype(value)>;
		if constexpr (std::is_same_v<T, float>)
		{
			CHECK_MIN_MAX()
		}
		else if constexpr (std::is_same_v<T, std::pair<float, float>>)
		{
			CHECK_MIN_MAX(first)
				CHECK_MIN_MAX(second)
		}
#undef CHECK_MIN_MAX
	};

#define RECALCULATE(z, is_min, elem) recalculate.operator()<elem, is_min>();
	BOOST_PP_SEQ_FOR_EACH(RECALCULATE, true, PARAMS_NAMES)
	BOOST_PP_SEQ_FOR_EACH(RECALCULATE, false, PARAMS_NAMES)
#undef RECALCULATE

	recalculate_summary(measure, item_row, std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[4]) -= measure->m_pores.get<MeasureWindow::tag_multiset>().count(pore_id));
}

void StatisticWindow::PoresStatisticList::on_pore_recovered(uint32_t pore_id)
{
	MeasureWindow* measure = parent_statwindow->parent_frame->m_measure;
	container_t::iterator item_iter;
	if (parent_statwindow->settings_window->m_checkBox_deleted->IsChecked())
	{
		long item_index = pore_id + 3;
		item_iter = container.begin() + item_index;
		attributes.erase(pore_id);
	}
	else
	{
		pore_statistic_collector psc(measure, container);
		psc.it = measure->m_pores.get<MeasureWindow::tag_multiset>().find(pore_id);
		psc.collect(pore_id);
		if (std::ptrdiff_t offset = pore_id + 3 - std::distance(measure->m_deleted_pores.begin(), measure->m_deleted_pores.lower_bound(pore_id)); offset < container.size()-1)
		{
			row_t row = std::move(container.back());
			for (std::ptrdiff_t i = container.size() - 1; i > offset;)
				container[i] = std::move(container[--i]);
			container[offset] = std::move(row);
			item_iter = container.begin() + offset;
		}
	}

	auto recalculate = [this, item_iter]<uint8_t param_name, bool is_min>()
	{
#define CHECK_MIN_MAX(...) \
			if (float item_value = std::get<param_name>(*item_iter)##__VA_OPT__(.)__VA_ARGS__; conditional<is_min>(item_value < value##__VA_OPT__(.)__VA_ARGS__, \
				item_value > value##__VA_OPT__(.)__VA_ARGS__)) [[unlikely]] \
			{ \
				value##__VA_OPT__(.)__VA_ARGS__ = item_value; \
			}

		auto& value = std::get<param_name>(container[conditional<is_min>(2, 3)]);
		using T = std::remove_reference_t<decltype(value)>;
		if constexpr (std::is_same_v<T, float>)
		{
			CHECK_MIN_MAX()
		}
		else if constexpr (std::is_same_v<T, std::pair<float, float>>)
		{
			CHECK_MIN_MAX(first)
			CHECK_MIN_MAX(second)
		}
#undef CHECK_MIN_MAX
	};

#define RECALCULATE(z, is_min, elem) recalculate.operator()<elem, is_min>();
	BOOST_PP_SEQ_FOR_EACH(RECALCULATE, true, PARAMS_NAMES)
	BOOST_PP_SEQ_FOR_EACH(RECALCULATE, false, PARAMS_NAMES)
#undef RECALCULATE

	recalculate_summary(measure, *item_iter, std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[4]) += measure->m_pores.get<MeasureWindow::tag_multiset>().count(pore_id));
}

void StatisticWindow::PoresStatisticList::set_columns_width()
{
	for (uint8_t i : {PORES_PARAMS})
		if (i != ID && i != CENTROID)
			SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);
		else
			SetColumnWidth(i, wxLIST_AUTOSIZE);
}

void StatisticWindow::PoresStatisticList::recalculate_summary(MeasureWindow* measure, row_t& item_row, float pores_square)
{
	// TODO : сделать оптимизацию с attributes.contains

	uint32_t pores_num = measure->pores_count - measure->m_deleted_pores.size();
	float
		&square_mean = std::get<SQUARE>(container[0]),
		&perimeter_mean = std::get<PERIMETER>(container[0]), &diameter_mean = std::get<DIAMETER>(container[0]),
		&sum_x_mean = std::get<CENTROID>(container[0]).first, &sum_y_mean = std::get<CENTROID>(container[0]).second,
		&lenght_oy_mean = std::get<LENGTH_OY>(container[0]), width_ox_mean = std::get<WIDTH_OX>(container[0]),
		&shape_mean = std::get<SHAPE>(container[0]), elongation_mean = std::get<ELONGATION>(container[0]);

#define PARAMS_VAARGS (square_mean),(perimeter_mean),(diameter_mean),(sum_x_mean, sum_y_mean),(lenght_oy_mean),(width_ox_mean),(shape_mean),(elongation_mean)
#define PARAMS BOOST_PP_VARIADIC_TO_SEQ(PARAMS_VAARGS)
#define RECALCULATE(z, n, __) \
		{ \
			BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), \
				((BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(n, PARAMS)) *= (pores_num + 1)) -= std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(item_row)) /= pores_num; \
				double deviation = 0;, \
				((BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(n, PARAMS)) *= (pores_num + 1)) -= std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(item_row).first) /= pores_num; \
				((BOOST_PP_TUPLE_ELEM(1, BOOST_PP_SEQ_ELEM(n, PARAMS)) *= (pores_num + 1)) -= std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(item_row).second) /= pores_num; \
				double deviation_x = 0; double deviation_y = 0;) \
			for (auto it = container.begin() + 4, end = container.end(); it != end; ++it) \
				if (!attributes.contains(std::get<ID>(*it))) \
					BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), \
						deviation += std::pow(BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(n, PARAMS)) - std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it), 2);, \
						{ \
							deviation_x += std::pow(BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(n, PARAMS)) - std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it).first, 2); \
							deviation_y += std::pow(BOOST_PP_TUPLE_ELEM(1, BOOST_PP_SEQ_ELEM(n, PARAMS)) - std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it).second, 2); \
						}) \
			std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(container[1]) = \
				BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), std::sqrtf(deviation/(pores_num-1));, (std::pair{std::sqrtf(deviation_x/(pores_num-1)), std::sqrtf(deviation_y/(pores_num-1))});) \
		}
		BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(PARAMS), RECALCULATE, ~)
#undef RECALCULATE
#undef PARAMS_VAARGS
#undef PARAMS

	uint32_t all_size = measure->width*measure->height;
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[2]) = pores_num;
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[3]) = pores_square/float(all_size);
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[4]) = pores_square;
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[5]) = all_size - pores_square;

	SetItemCount((parent_statwindow->settings_window->m_checkBox_deleted->IsChecked() ? measure->pores_count : pores_num) + 4);
	parent_statwindow->common_statistic_list->SetColumnWidth(1, wxLIST_AUTOSIZE);
	set_columns_width();
	parent_statwindow->common_statistic_list->Refresh();
	Refresh();
	parent_statwindow->common_statistic_list->Update();
	Update();
}

// DistributionWindow

wxBEGIN_EVENT_TABLE(StatisticWindow::DistributionWindow, wxWindow)
	EVT_PAINT(StatisticWindow::DistributionWindow::OnPaint)
wxEND_EVENT_TABLE()

StatisticWindow::DistributionWindow::DistributionWindow(wxWindow* parent) : wxWindow(parent, wxID_ANY)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void StatisticWindow::DistributionWindow::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);
	dc.Clear();
}

// SettingsWindow

wxWindowID
	StatisticWindow::SettingsWindow::checkBox_color_id = wxWindow::NewControlId(),
	StatisticWindow::SettingsWindow::checkBox_deleted_id = wxWindow::NewControlId();

wxBEGIN_EVENT_TABLE(StatisticWindow::SettingsWindow, wxWindow)
	EVT_CHECKBOX(StatisticWindow::SettingsWindow::checkBox_color_id, StatisticWindow::SettingsWindow::OnDistributionColor)
	EVT_CHECKBOX(StatisticWindow::SettingsWindow::checkBox_deleted_id, StatisticWindow::SettingsWindow::OnShowDeleted)
wxEND_EVENT_TABLE()

StatisticWindow::SettingsWindow::SettingsWindow(wxWindow* parent) : wxWindow(parent, wxID_ANY), parent_statwindow(static_cast<StatisticWindow*>(parent))
{
	SetBackgroundColour(*wxLIGHT_GREY);

	wxStaticBoxSizer* static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Настройка")), wxVERTICAL);

	m_checkBox_color = new wxCheckBox(this, checkBox_color_id, wxT("Раскраска пор под цвет интервалов распределения"));
	static_box_sizer->Add( m_checkBox_color, 0, wxALL, 5 );

	m_checkBox_deleted = new wxCheckBox(this, checkBox_deleted_id, wxT("Показывать в списке удалённые поры"));
	static_box_sizer->Add( m_checkBox_deleted, 0, wxALL, 5 );

	SetSizer(static_box_sizer);
}

void StatisticWindow::SettingsWindow::OnDistributionColor(wxCommandEvent& event)
{
	;
}

void StatisticWindow::SettingsWindow::OnShowDeleted(wxCommandEvent& event)
{
	parent_statwindow->pores_statistic_list->attributes.clear();
	if (MeasureWindow* measure = parent_statwindow->parent_frame->m_measure; event.IsChecked())
	{
		pore_statistic_collector psc(measure, parent_statwindow->pores_statistic_list->container);
		for (uint32_t id : measure->m_deleted_pores)
		{
			psc.it = measure->m_pores.get<MeasureWindow::tag_multiset>().find(id);
			psc.collect(id);

			wxItemAttr attr;
			attr.SetBackgroundColour(0xAAAAEE);
			parent_statwindow->pores_statistic_list->attributes.insert({id, attr});
		}
		std::sort(parent_statwindow->pores_statistic_list->container.begin() + 4, parent_statwindow->pores_statistic_list->container.end(),
			[](const PoresStatisticList::row_t& lhs, const PoresStatisticList::row_t& rhs){return std::get<PoresStatisticList::ID>(lhs) < std::get<PoresStatisticList::ID>(rhs);});
		
		parent_statwindow->pores_statistic_list->SetItemCount(parent_statwindow->pores_statistic_list->GetItemCount() + measure->m_deleted_pores.size());
	}
	else
	{
		std::erase_if(parent_statwindow->pores_statistic_list->container, [measure](PoresStatisticList::row_t& row){return measure->m_deleted_pores.contains(std::get<PoresStatisticList::ID>(row));});
		parent_statwindow->pores_statistic_list->SetItemCount(parent_statwindow->pores_statistic_list->GetItemCount() - measure->m_deleted_pores.size());
	}
	parent_statwindow->pores_statistic_list->set_columns_width();
	parent_statwindow->pores_statistic_list->Refresh();
	parent_statwindow->pores_statistic_list->Update();
}