#include "StatisticWindow.h"
#include "Frame.h"
#include "Utils.h"
#include <wx/dcbuffer.h>
#include <wx/statbox.h>
#include <charconv>
#include <boost/preprocessor.hpp>

// Auxiliary

struct pore_statistic_collector
{
	using multiset_iterator_t = MeasureWindow::pores_container::index<MeasureWindow::tag_multiset>::type::iterator;
	using hashset_iterator_t = MeasureWindow::pores_container::index<MeasureWindow::tag_hashset>::type::iterator;

	MeasureWindow* measure;
	StatisticWindow::pores_statistic_container& container;
	multiset_iterator_t it, end;
	hashset_iterator_t tmp_it, end_it;
	uint32_t square, perimeter, sum_x, sum_y, min_x, max_x, min_y, max_y, lenght_oy, width_ox;
	float shape, diameter, elongation, centroid_x, centroid_y;

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
		centroid_x = sum_x/float(square);
		centroid_y = sum_y/float(square);
		container.emplace_back(id, square, perimeter, diameter, centroid_x, centroid_y, 0, 0, lenght_oy, width_ox, 0, 0, shape, elongation);
	}
};

// Актуальные вычисляемые параметры
#define PARAMS_NAMES (SQUARE)(PERIMETER)(DIAMETER)(CENTROID_X)(CENTROID_Y)(LENGTH_OY)(WIDTH_OX)(SHAPE)(ELONGATION)
#define PARAMS pores_square, perimeter_mean, diameter_mean, centroid_x_mean, centroid_y_mean, lenght_oy_mean, width_ox_mean, shape_mean, elongation_mean
#define MEAN_PARAMS_DECLARATOR(s, _, n, elem) &elem = std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(container[0])BOOST_PP_COMMA_IF(BOOST_PP_LESS(BOOST_PP_INC(n), BOOST_PP_VARIADIC_SIZE(PARAMS)))
#define RECALCULATE_MEAN_DEVIATION(z, n, delete_or_recover) \
	{ \
		((BOOST_PP_VARIADIC_ELEM(n, PARAMS) *= (parent_statwindow->num_considered BOOST_PP_IF(delete_or_recover,+,-) 1)) BOOST_PP_CAT(BOOST_PP_IF(delete_or_recover,-,+), =) std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(item_row)) /= parent_statwindow->num_considered; \
		double deviation = 0; \
		for (auto it = container.begin() + 4, end = container.end(); it != end; ++it) \
			if (!attributes.contains(std::get<ID>(*it))) \
				deviation += std::pow(BOOST_PP_VARIADIC_ELEM(n, PARAMS) - std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it), 2); \
		std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(container[1]) = std::sqrtf(deviation/(parent_statwindow->num_considered-1)); \
	}
#define RECALCULATE(delete_or_recover) \
	{ \
		if (parent_statwindow->num_considered > 1) \
		{ \
			BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(PARAMS_NAMES), CHECK_MIN_MAX, 1) \
			BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(PARAMS_NAMES), CHECK_MIN_MAX, 0) \
			float BOOST_PP_SEQ_FOR_EACH_I(MEAN_PARAMS_DECLARATOR, ~, BOOST_PP_VARIADIC_TO_SEQ(PARAMS)); \
			BOOST_PP_REPEAT(BOOST_PP_VARIADIC_SIZE(PARAMS), RECALCULATE_MEAN_DEVIATION, delete_or_recover) \
		} \
		else if (parent_statwindow->num_considered == 1) \
		{ \
			BOOST_PP_EXPR_IF(delete_or_recover, for (auto it = container.begin() + 4, end = container.end(); it != end; ++it) if (!attributes.contains(std::get<ID>(*it))) {) \
			container[0] = BOOST_PP_IF(delete_or_recover, *it; break;}, item_row;) \
			std::get<PoresStatisticList::ID>(container[0]) = 0; \
			container[1] = PoresStatisticList::row_t{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; \
			container[2] = container[0]; \
			container[3] = container[0]; \
		} \
	}

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
	pores_statistic_list->AppendColumn(wxT("Координата X"));
	pores_statistic_list->AppendColumn(wxT("Координата Y"));
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

	constexpr float fmax = std::numeric_limits<float>::max();
	common_statistic_list->container[0] = CommonStatisticList::row_t{wxString{"Ширина"}, 0};
	common_statistic_list->container[1] = CommonStatisticList::row_t{wxString{"Высота"}, 0};
	common_statistic_list->container[2] = CommonStatisticList::row_t{wxString{"Количество пор"}, 0};
	common_statistic_list->container[3] = CommonStatisticList::row_t{wxString{"Удельное количество пор"}, 0};
	common_statistic_list->container[4] = CommonStatisticList::row_t{wxString{"Площадь пор"}, 0};
	common_statistic_list->container[5] = CommonStatisticList::row_t{wxString{"Площадь матрицы"}, 0};
	pores_statistic_list->container.resize(2, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
	pores_statistic_list->container.push_back({0, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax});
	pores_statistic_list->container.push_back({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
	common_statistic_list->SetItemCount(6);
	pores_statistic_list->SetItemCount(4);

	common_statistic_list->SetColumnWidth(0, wxLIST_AUTOSIZE);
	common_statistic_list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
	pores_statistic_list->SetColumnWidth(0, wxLIST_AUTOSIZE);
	for (uint8_t i = 1, count = pores_statistic_list->GetColumnCount(); i < count; ++i)
		pores_statistic_list->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);

	m_aui->Update();
}

void StatisticWindow::CollectStatistic()
{
	MeasureWindow* measure = parent_frame->m_measure;
	bool collect_deleted = settings_window->m_checkBox_deleted->IsChecked();
	uint32_t pores_square = 0;
	double perimeter_mean = 0, diameter_mean = 0, centroid_x_mean = 0, centroid_y_mean = 0, lenght_oy_mean = 0, width_ox_mean = 0, shape_mean = 0, elongation_mean = 0;
	num_considered = measure->pores_count - measure->m_deleted_pores.size();

	pores_statistic_list->attributes.clear();
	pores_statistic_list->container.clear();
	pores_statistic_list->container.reserve((collect_deleted ? measure->pores_count : num_considered) + 4);
	pores_statistic_list->container.resize(2, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
	constexpr float fmax = std::numeric_limits<float>::max();
	pores_statistic_list->container.push_back({0, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax});
	pores_statistic_list->container.push_back({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
	pores_statistic_list->SetItemCount((collect_deleted ? measure->pores_count : num_considered) + 4);

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
				centroid_x_mean += psc.centroid_x;
				centroid_y_mean += psc.centroid_y;
				lenght_oy_mean += psc.lenght_oy;
				width_ox_mean += psc.width_ox;
				shape_mean += psc.shape;
				elongation_mean += psc.elongation;

#define PARAMS_VAARGS_PSC psc.square, psc.perimeter, psc.diameter, psc.centroid_x, psc.centroid_y, psc.lenght_oy, psc.width_ox, psc.shape, psc.elongation
#define SET_MIN_MAX(z, n, flag) \
					{ \
						auto& value = std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(pores_statistic_list->container[flag]); \
						if (value BOOST_PP_IF(BOOST_PP_EQUAL(flag, 2), >, <) BOOST_PP_VARIADIC_ELEM(n, PARAMS_VAARGS_PSC)) [[unlikely]] \
							value = BOOST_PP_VARIADIC_ELEM(n, PARAMS_VAARGS_PSC); \
					}
				BOOST_PP_REPEAT(BOOST_PP_VARIADIC_SIZE(PARAMS_VAARGS_PSC), SET_MIN_MAX, 2)
				BOOST_PP_REPEAT(BOOST_PP_VARIADIC_SIZE(PARAMS_VAARGS_PSC), SET_MIN_MAX, 3)
#undef PARAMS_VAARGS_PSC
#undef PARAMS_PSC
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
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[0]) = measure->width;
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[1]) = measure->height;
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[2]) = num_considered;
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[3]) = pores_square/float(all_size);
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[4]) = pores_square;
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[5]) = all_size - pores_square;

#define SET_MEAN_DEVIATION(z, n, set_deviation) \
		{ \
			auto& mean = std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(pores_statistic_list->container[0]); \
			mean = BOOST_PP_VARIADIC_ELEM(n, PARAMS)/float(num_considered); \
			BOOST_PP_IF(set_deviation, \
				double deviation = 0; \
				for (auto it = pores_statistic_list->container.begin() + 4, end = pores_statistic_list->container.end(); it != end; ++it) \
					if (!pores_statistic_list->attributes.contains(std::get<PoresStatisticList::ID>(*it))) \
						deviation += std::pow(mean - std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it), 2); \
				std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(pores_statistic_list->container[1]) = std::sqrtf(deviation/(num_considered-1));, \
				std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(pores_statistic_list->container[1]) = 1; \
			) \
		}
	if (num_considered > 1)
	{
		BOOST_PP_REPEAT(BOOST_PP_VARIADIC_SIZE(PARAMS), SET_MEAN_DEVIATION, 1)
	}
	else if (num_considered == 1)
	{
		BOOST_PP_REPEAT(BOOST_PP_VARIADIC_SIZE(PARAMS), SET_MEAN_DEVIATION, 0)
	}

	pores_statistic_list->set_columns_width();
	common_statistic_list->Refresh();
	pores_statistic_list->Refresh();
	distribution_window->Refresh();
	distribution_window->Update();
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

StatisticWindow::CommonStatisticList::CommonStatisticList(StatisticWindow* parent)
	: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL | wxLC_HRULES | wxLC_VRULES), parent_statwindow(parent)
{
	EnableAlternateRowColours();
}

wxString StatisticWindow::CommonStatisticList::OnGetItemText(long item, long column) const
{
	char buf[16];
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
	EVT_LIST_COL_CLICK(wxID_ANY, StatisticWindow::PoresStatisticList::OnColumnClick)
wxEND_EVENT_TABLE()

StatisticWindow::PoresStatisticList::PoresStatisticList(StatisticWindow* parent)
	: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL | wxLC_HRULES | wxLC_VRULES), parent_statwindow(parent)
{
	EnableAlternateRowColours();
}

wxString StatisticWindow::PoresStatisticList::OnGetItemText(long item, long column) const
{
#define CASE_NUMBER(value) case value: if (parent_statwindow->num_considered > 0 || attributes.contains(item - 3)) {*std::to_chars(buf, buf + 16, std::get<value>(container[item])).ptr = '\0'; return {buf};} else return {'-'};

	char buf[16];
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
	CASE_NUMBER(CENTROID_X)
	CASE_NUMBER(CENTROID_Y)
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
			float pore_x = std::get<CENTROID_X>(container[item_index]), pore_y = std::get<CENTROID_Y>(container[item_index]);
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

void StatisticWindow::PoresStatisticList::OnColumnClick(wxListEvent& event)
{
	parent_statwindow->distribution_window->column_index = event.GetColumn();
	parent_statwindow->distribution_window->Refresh();
	parent_statwindow->distribution_window->Update();
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
	--parent_statwindow->num_considered;

#define CHECK_MIN_MAX(z, n, is_min) \
		{ \
			auto& value = std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(container[BOOST_PP_IF(is_min, 2, 3)]); \
			if (std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(item_row) BOOST_PP_IF(is_min, <=, >=) value) [[unlikely]] \
			{ \
				value = std::numeric_limits<float>::BOOST_PP_IF(is_min, max, min)(); \
				for (auto it = container.begin() + 4, end = container.end(); it != end; ++it) \
					if (!attributes.contains(std::get<ID>(*it))) \
						if (auto tmp_value = std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it); tmp_value BOOST_PP_IF(is_min, <, >) value) [[unlikely]] \
							value = tmp_value; \
			} \
		}
	RECALCULATE(1)
#undef CHECK_MIN_MAX

	after_changes(measure, std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[4]) -= measure->m_pores.get<MeasureWindow::tag_multiset>().count(pore_id));
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
		std::size_t offset = pore_id + 3 - std::distance(measure->m_deleted_pores.begin(), measure->m_deleted_pores.lower_bound(pore_id));
		if (offset < container.size()-1)
		{
			row_t row = std::move(container.back());
			for (std::size_t i = container.size() - 1; i > offset; --i)
				container[i] = std::move(container[i - 1]);
			container[offset] = std::move(row);
		}
		item_iter = container.begin() + offset;
	}
	row_t& item_row = *item_iter;
	++parent_statwindow->num_considered;

#define CHECK_MIN_MAX(z, n, is_min) \
		{ \
			auto& value = std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(container[BOOST_PP_IF(is_min, 2, 3)]); \
			if (float item_value = std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(item_row); item_value BOOST_PP_IF(is_min, <, >) value) [[unlikely]] \
				value = item_value; \
		}
	RECALCULATE(0)
#undef CHECK_MIN_MAX

	after_changes(measure, std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[4]) += measure->m_pores.get<MeasureWindow::tag_multiset>().count(pore_id));
}

void StatisticWindow::PoresStatisticList::set_columns_width()
{
	for (uint8_t i : {PORES_PARAMS})
		if (i != ID)
			SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);
		else
			SetColumnWidth(i, wxLIST_AUTOSIZE);
}

void StatisticWindow::PoresStatisticList::after_changes(MeasureWindow* measure, float pores_square)
{
	uint32_t all_size = measure->width*measure->height;
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[2]) = parent_statwindow->num_considered;
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[3]) = pores_square/float(all_size);
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[4]) = pores_square;
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[5]) = all_size - pores_square;

	SetItemCount((parent_statwindow->settings_window->m_checkBox_deleted->IsChecked() ? measure->pores_count : parent_statwindow->num_considered) + 4);
	//parent_statwindow->common_statistic_list->SetColumnWidth(1, wxLIST_AUTOSIZE);
	//set_columns_width();
	parent_statwindow->common_statistic_list->Refresh();
	parent_statwindow->distribution_window->Refresh();
	Refresh();
	parent_statwindow->common_statistic_list->Update();
	parent_statwindow->distribution_window->Update();
	Update();
}

// DistributionWindow

wxBEGIN_EVENT_TABLE(StatisticWindow::DistributionWindow, wxWindow)
	EVT_PAINT(StatisticWindow::DistributionWindow::OnPaint)
wxEND_EVENT_TABLE()

StatisticWindow::DistributionWindow::DistributionWindow(StatisticWindow* parent) : wxWindow(parent, wxID_ANY), parent_statwindow(parent)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void StatisticWindow::DistributionWindow::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);
	dc.Clear();
	int width = GetSize().GetWidth(), height = GetSize().GetHeight();
	double scale = 12.0/16.0;
	wxPen pen{wxColour{0ul}, 3};
	std::unique_ptr<wxGraphicsContext> gc{wxGraphicsContext::Create(dc)};
	gc->SetPen(pen);
	wxGraphicsMatrix transform_matrix = gc->CreateMatrix(scale, 0, 0, -scale, width/16.0, 15.0*height/16.0), default_matrix = gc->CreateMatrix(1, 0, 0, 1, 0, 0);

	wxGraphicsPath path_axis = gc->CreatePath();
	path_axis.MoveToPoint(0.0, 0.0);
	path_axis.AddLineToPoint(0.0, height*(1 + 2/16.0/scale));
	path_axis.AddLineToPoint(-width/32.0, height*(1 + 1/16.0/scale));
	path_axis.MoveToPoint(0.0, height*(1 + 2/16.0/scale));
	path_axis.AddLineToPoint(width/32.0, height*(1 + 1/16.0/scale));
	path_axis.MoveToPoint(0.0, 0.0);
	path_axis.AddLineToPoint(width*(1 + 2/16.0/scale), 0.0);
	path_axis.AddLineToPoint(width*(1 + 1/16.0/scale), -height/32.0);
	path_axis.MoveToPoint(width*(1 + 2/16.0/scale), 0.0);
	path_axis.AddLineToPoint(width*(1 + 1/16.0/scale), height/32.0);

	if (column_index != PoresStatisticList::ID)
	{
		uint8_t k = uint8_t(std::ceil(1 + 3.322 * std::log10(std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[2]))));
		float min_value = get_tuple_element<float>(parent_statwindow->pores_statistic_list->container[2], column_index), max_value = get_tuple_element<float>(parent_statwindow->pores_statistic_list->container[3], column_index),
			values_interval = (max_value - min_value) / k, draw_interval = width / float(k);
		//float mean_point = (mean - min_x) / (max_x + interval - min_x);
		std::vector<uint32_t> columns_counts(k+1, 0);
		for (auto it = parent_statwindow->pores_statistic_list->container.begin() + 4, end = parent_statwindow->pores_statistic_list->container.end(); it != end; ++it)
			if (!parent_statwindow->pores_statistic_list->attributes.contains(std::get<PoresStatisticList::ID>(*it)))
				++columns_counts[uint8_t(std::floor((get_tuple_element<float>(*it, column_index) - min_value)/values_interval))];
		columns_counts[k-1] += columns_counts[k];
		uint32_t max_count = columns_counts[0];
		for (uint8_t i = 1; i < k; ++i)
			if (columns_counts[i] > max_count)
				max_count = columns_counts[i];

		wxGraphicsPath path_columns = gc->CreatePath();
		char buf[16];
		std::vector<wxString> strings;
		strings.reserve(k+1);
		double label_max_width = scale*draw_interval, label_max_height = height/16.0;
		uint8_t max_lenght = 0, index_for_set_font;
		*std::to_chars(buf, buf + 16, max_count).ptr = '\0';
		set_correct_font_size(gc.get(), {buf}, 10, label_max_width, label_max_height);
		double text_width_tmp, text_height_tmp;
		for (uint8_t i = 0; i < k; ++i)
		{
			double h = height*columns_counts[i]/double(max_count);
			gc->SetTransform(transform_matrix);
			path_columns.AddRectangle(i*draw_interval, 0, draw_interval, h);
			*std::to_chars(buf, buf + 16, columns_counts[i]).ptr = '\0';
			wxString count_str{buf};
			gc->GetTextExtent(count_str, &text_width_tmp, &text_height_tmp);
			gc->SetTransform(default_matrix);
			gc->DrawText(count_str, width/16.0 + scale*(draw_interval*(i + 0.5)) - text_width_tmp/2, height*(1 - 1/16.0) - scale*h - text_height_tmp);
			*std::to_chars(buf, buf + 16, std::roundf(100*(values_interval*i + min_value))/100).ptr = '\0';
			if (uint8_t len = std::strlen(buf); len > max_lenght)
				index_for_set_font = i;
			strings.emplace_back(buf);
		}
		*std::to_chars(buf, buf + 16, std::roundf(100*(max_value))/100).ptr = '\0';
		strings.emplace_back(buf);
		label_max_width *= 0.9f;
		set_correct_font_size(gc.get(), strings[index_for_set_font], 10, label_max_width, label_max_height);
		for (auto begin = strings.begin() + 1, end = strings.end(), it = begin; it != end; ++it)
		{
			gc->GetTextExtent(*it, &text_width_tmp, &text_height_tmp);
			if (text_width_tmp >= label_max_width) [[unlikely]]
			{
				wxString copy{*it};
				if (int point_pos = copy.Find('.'); point_pos != wxNOT_FOUND)
				{
					do
					{
						copy.RemoveLast();
						gc->GetTextExtent(copy, &text_width_tmp, &text_height_tmp);
					} while (text_width_tmp >= label_max_width);
					if (copy.size() >= unsigned(point_pos))
					{
						if (copy.size() == point_pos + 1) [[unlikely]]
							copy.RemoveLast();
						*it = std::move(copy);
						continue;
					}
					else
						it->Truncate(point_pos);
				}
				set_correct_font_size(gc.get(), *it, 10, label_max_width, label_max_height);
			}
		}
		//gc->SetTransform(default_matrix);
		for (uint8_t i = 0; i <= k; ++i)
		{
			gc->GetTextExtent(strings[i], &text_width_tmp, &text_height_tmp);
			gc->DrawText(strings[i], width/16.0 + scale*(draw_interval*i - text_width_tmp/2), height*(1 - 1/16.0));
		}

		wxListItem column_info;
		column_info.SetMask(wxLIST_MASK_TEXT);
		parent_statwindow->pores_statistic_list->GetColumn(column_index, column_info);
		set_correct_font_size(gc.get(), column_info.GetText(), 16, scale*width, height*2/16.0);
		double text_width, text_height;
		gc->GetTextExtent(column_info.GetText(), &text_width, &text_height);
		gc->DrawText(column_info.GetText(), (width - text_width)/2, 0);

		gc->SetTransform(transform_matrix);
		gc->SetBrush(*wxRED_BRUSH);
		gc->DrawPath(path_columns);
	}
	else
		gc->SetTransform(transform_matrix);
	gc->StrokePath(path_axis);
}

void StatisticWindow::DistributionWindow::set_correct_font_size(wxGraphicsContext* gc, const wxString& text, int initial_size, double max_width, double max_height)
{
	wxFont font{initial_size, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_SEMIBOLD};
	gc->SetFont(font, {0ul});
	double text_width, text_height;
	gc->GetTextExtent(text, &text_width, &text_height);
	if (text_height >= max_height || text_width >= max_width)
		do
		{
			font.SetPointSize(--initial_size);
			gc->SetFont(font, {0ul});
			gc->GetTextExtent(text, &text_width, &text_height);
		} while (text_height >= max_height || text_width >= max_width);
	else
	{
		do
		{
			font.SetPointSize(++initial_size);
			gc->SetFont(font, {0ul});
			gc->GetTextExtent(text, &text_width, &text_height);
		} while (text_height < max_height && text_width < max_width);
		font.SetPointSize(--initial_size);
		gc->SetFont(font, {0ul});
	}
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
	//parent_statwindow->pores_statistic_list->set_columns_width();
	parent_statwindow->pores_statistic_list->Refresh();
	parent_statwindow->pores_statistic_list->Update();
}