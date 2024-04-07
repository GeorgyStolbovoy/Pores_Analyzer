#include "Frame.h"
#include "Utils.h"
#include <wx/dcbuffer.h>
#include <wx/statbox.h>
#include <charconv>

// Auxiliary

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
#define SLIDERS_ACTUAL BOOST_PP_SEQ_TRANSFORM(NAME_SLIDER, ~, PARAMS_NAMES)

// StatisticWindow

wxBEGIN_EVENT_TABLE(StatisticWindow, wxWindow)
	EVT_PAINT(StatisticWindow::OnPaint)
wxEND_EVENT_TABLE()

StatisticWindow::StatisticWindow()
	: wxWindow(Frame::frame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	
	m_aui = new Aui{this};

	common_statistic_list = new CommonStatisticList(this);
	common_statistic_list ->AppendColumn(wxT("Параметр"));
	common_statistic_list ->AppendColumn(wxT("Значение"));
	m_aui->AddPane(common_statistic_list, wxAuiPaneInfo{}.Caption(wxT("Общая характеристика")).CloseButton(false).Top().MaximizeButton().PaneBorder(true).Name(wxT("Common")));

	pores_statistic_list = new PoresStatisticList(this);
#define APPEND_COLUMN(z, data, name) pores_statistic_list->AppendColumn(wxT(name));
	BOOST_PP_SEQ_FOR_EACH(APPEND_COLUMN, ~, PORES_PARAMS_NAMES)
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

void StatisticWindow::collect_statistic_for_pore(uint32_t id)
{
	auto it = Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_multiset>().find(id), end = Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_multiset>().end();
	uint32_t square = 0, perimeter = 0, sum_x = 0, sum_y = 0, min_x = Frame::frame->m_measure->width - 1, max_x = 0, min_y = Frame::frame->m_measure->height - 1, max_y = 0;
	MeasureWindow::pores_container::index<MeasureWindow::tag_hashset>::type::iterator tmp_it, end_it = Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_hashset>().end();
	do
	{
		++square;
		sum_x += it->second.first;
		sum_y += it->second.second;
		if (min_x > uint32_t(it->second.first)) [[unlikely]] min_x = it->second.first;
		if (max_x < uint32_t(it->second.first)) [[unlikely]] max_x = it->second.first;
		if (min_y > uint32_t(it->second.second)) [[unlikely]] min_y = it->second.second;
		if (max_y < uint32_t(it->second.second)) [[unlikely]] max_y = it->second.second;
		if (Frame::frame->m_measure->m_boundary_pixels.contains(it->second)) [[unlikely]]
		{
#define INC_IF_BOUNDARY(dx, dy) if (auto tmp_it = Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_hashset>().find(MeasureWindow::coord_t{it->second.first dx, it->second.second dy}); tmp_it == end_it || tmp_it->first != id) ++perimeter
			INC_IF_BOUNDARY(-1,);
			INC_IF_BOUNDARY(,+1);
			INC_IF_BOUNDARY(+1,);
			INC_IF_BOUNDARY(,-1);
		}
	} while (++it != end && it->first == id);
	float lenght_oy = max_y - min_y + 1;
	float width_ox = max_x - min_x + 1;
	float shape = 4*M_PI*square/(perimeter*perimeter);
	float diameter = std::sqrtf(4*square/M_PI);
	float elongation = 1/shape;
	float centroid_x = sum_x/float(square);
	float centroid_y = sum_y/float(square);
	Frame::frame->m_statistic->pores_statistic_list->container.emplace_back(id, square, perimeter, diameter, centroid_x, centroid_y, 0, 0, lenght_oy, width_ox, 0, 0, shape, elongation);
}

void StatisticWindow::CollectStatistic()
{
	MeasureWindow* measure = Frame::frame->m_measure;
	uint32_t pores_square = 0;
	double perimeter_mean = 0, diameter_mean = 0, centroid_x_mean = 0, centroid_y_mean = 0, lenght_oy_mean = 0, width_ox_mean = 0, shape_mean = 0, elongation_mean = 0;
	uint32_t num_deleted = measure->m_deleted_pores.size(), num_filtered = measure->m_filtered_pores.size();
	num_considered = measure->pores_count - num_deleted - num_filtered;

	pores_statistic_list->attributes.clear();
	pores_statistic_list->container.clear();
	pores_statistic_list->container.reserve(measure->pores_count);
	pores_statistic_list->container.resize(2, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
	constexpr float fmax = std::numeric_limits<float>::max();
	pores_statistic_list->container.push_back({0, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax});
	pores_statistic_list->container.push_back({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
	pores_statistic_list->set_item_count();

	{
		auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().begin(), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
		auto end_it = Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_hashset>().end(), tmp_it = end_it;
		do
		{
			uint32_t id = it->first;
			uint32_t square = 0, perimeter = 0, sum_x = 0, sum_y = 0, min_x = Frame::frame->m_measure->width - 1, max_x = 0, min_y = Frame::frame->m_measure->height - 1, max_y = 0;
			do
			{
				++square;
				sum_x += it->second.first;
				sum_y += it->second.second;
				if (min_x > uint32_t(it->second.first)) [[unlikely]] min_x = it->second.first;
				if (max_x < uint32_t(it->second.first)) [[unlikely]] max_x = it->second.first;
				if (min_y > uint32_t(it->second.second)) [[unlikely]] min_y = it->second.second;
				if (max_y < uint32_t(it->second.second)) [[unlikely]] max_y = it->second.second;
				if (Frame::frame->m_measure->m_boundary_pixels.contains(it->second)) [[unlikely]]
				{
					INC_IF_BOUNDARY(-1,);
					INC_IF_BOUNDARY(,+1);
					INC_IF_BOUNDARY(+1,);
					INC_IF_BOUNDARY(,-1);
				}
			} while (++it != end && it->first == id);
			uint32_t lenght_oy = max_y - min_y + 1;
			uint32_t width_ox = max_x - min_x + 1;
			float shape = 4*M_PI*square/(perimeter*perimeter);
			float diameter = std::sqrtf(4*square/M_PI);
			float elongation = 1/shape;
			float centroid_x = sum_x/float(square);
			float centroid_y = sum_y/float(square);
			pores_statistic_list->container.emplace_back(id, square, perimeter, diameter, centroid_x, centroid_y, 0, 0, lenght_oy, width_ox, 0, 0, shape, elongation);
			if (bool not_deleted = !measure->m_deleted_pores.contains(id), not_filtered = !measure->m_filtered_pores.contains(id); not_deleted && not_filtered)
			{
				pores_square += square;
				perimeter_mean += perimeter;
				diameter_mean += diameter;
				centroid_x_mean += centroid_x;
				centroid_y_mean += centroid_y;
				lenght_oy_mean += lenght_oy;
				width_ox_mean += width_ox;
				shape_mean += shape;
				elongation_mean += elongation;

#define COLLECTED_PARAMS square, perimeter, diameter, centroid_x, centroid_y, lenght_oy, width_ox, shape, elongation
#define SET_MIN_MAX(z, n, flag) \
					{ \
						auto& value = std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(pores_statistic_list->container[flag]); \
						if (value BOOST_PP_IF(BOOST_PP_EQUAL(flag, 2), >, <) BOOST_PP_VARIADIC_ELEM(n, COLLECTED_PARAMS)) [[unlikely]] \
							value = BOOST_PP_VARIADIC_ELEM(n, COLLECTED_PARAMS); \
					}
				BOOST_PP_REPEAT(BOOST_PP_VARIADIC_SIZE(COLLECTED_PARAMS), SET_MIN_MAX, 2)
				BOOST_PP_REPEAT(BOOST_PP_VARIADIC_SIZE(COLLECTED_PARAMS), SET_MIN_MAX, 3)
#undef COLLECTED_PARAMS
			}
			else
			{
				wxItemAttr attr;
				if (!not_deleted)
					attr.SetBackgroundColour(0xAAAAEE);
				else
					attr.SetBackgroundColour(0xAAEEEE);
				pores_statistic_list->attributes.insert({id, attr});
			}
		} while (it != end);
	}

	uint32_t all_size = measure->width*measure->height;
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[0]) = measure->width;
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[1]) = measure->height;
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[2]) = num_considered;
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[3]) = pores_square/float(all_size);
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[4]) = pores_square;
	std::get<CommonStatisticList::PARAM_VALUE>(common_statistic_list->container[5]) = all_size - pores_square;

#define SET_MEAN_DEVIATION(z, n, set_deviation) \
		{ \
			float& mean = std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(pores_statistic_list->container[0]); \
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

#define SET_SLIDERS_VALUES(z, data, i, s) measure->s->set_values( \
	std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(i, PORES_CALCULATING_PARAMS)>(pores_statistic_list->container[2]), \
	std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(i, PORES_CALCULATING_PARAMS)>(pores_statistic_list->container[3]));

	BOOST_PP_SEQ_FOR_EACH_I(SET_SLIDERS_VALUES, ~, SLIDERS)

	wxWindow* filters_pane = measure->GetWindowChild(measure->collapse_filter_id);
	filters_pane->Refresh();
	filters_pane->Update();
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

wxBEGIN_EVENT_TABLE(StatisticWindow::Aui, wxAuiManager)
	EVT_SIZE(StatisticWindow::Aui::SetPanesPositions)
	EVT_MOTION(StatisticWindow::Aui::SavePanePosition<&wxAuiManager::OnMotion>)
	EVT_LEFT_UP(StatisticWindow::Aui::SavePanePosition<&wxAuiManager::OnLeftUp>)
	EVT_AUI_PANE_BUTTON(StatisticWindow::Aui::OnPaneMinimaze)
wxEND_EVENT_TABLE()

void StatisticWindow::Aui::SetPanesPositions(wxSizeEvent& e)
{
	wxSize size = GetManagedWindow()->GetSize();
	for (uint8_t i = 0, dock_count = m_docks.GetCount(); i < dock_count; ++i)
	{
		wxAuiDockInfo& d = m_docks.Item(i);
		if (d.dock_direction == wxAUI_DOCK_TOP)
			d.size = size.GetHeight()*panes_positions.first - sashSize;
		else if (d.dock_direction == wxAUI_DOCK_RIGHT)
			d.size = size.GetWidth()*panes_positions.second - sashSize;
	}
	Update();
}

template <auto EvtHandler>
void StatisticWindow::Aui::SavePanePosition(wxMouseEvent& event)
{
	if (m_action == actionResize)
	{
		wxAuiDockInfo* dock;
		if (m_currentDragItem != -1)
			dock = m_uiParts.Item(m_currentDragItem).dock;
		else
			dock = m_actionPart->dock;
		int* new_size = &dock->size;
		int direction = dock->dock_direction;
		(static_cast<wxAuiManager*>(this)->*EvtHandler)(event);
		if (direction == wxAUI_DOCK_TOP)
			panes_positions.first = *new_size / float(GetManagedWindow()->GetSize().GetHeight() - sashSize);
		else if (direction == wxAUI_DOCK_RIGHT)
			panes_positions.second = *new_size / float(GetManagedWindow()->GetSize().GetWidth() - sashSize);
	}
	else
		event.Skip();
}

void StatisticWindow::Aui::OnPaneMinimaze(wxAuiManagerEvent& e)
{
	if (e.button == wxAUI_BUTTON_MAXIMIZE_RESTORE && e.pane->IsMaximized())
	{
		wxAuiManager::OnPaneButton(e);
		wxSize size = GetManagedWindow()->GetSize();
		for (uint8_t i = 0, dock_count = m_docks.GetCount(); i < dock_count; ++i)
		{
			wxAuiDockInfo& d = m_docks.Item(i);
			if (d.dock_direction == wxAUI_DOCK_TOP)
				d.size = size.GetHeight()*panes_positions.first - sashSize;
			else if (d.dock_direction == wxAUI_DOCK_RIGHT)
				d.size = size.GetWidth()*panes_positions.second - sashSize;
		}
		Update();
	}
	else
		e.Skip();
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

std::size_t StatisticWindow::PoresStatisticList::item_to_index(long item) const
{
	bool consider_deleted = !parent_statwindow->settings_window->m_checkBox_deleted->IsChecked() && !Frame::frame->m_measure->m_deleted_pores.empty(),
		consider_filtered = !parent_statwindow->settings_window->m_checkBox_filtered->IsChecked() && !Frame::frame->m_measure->m_filtered_pores.empty();
	if (consider_deleted && consider_filtered)
	{
		CONVERTER_BODY(item, 1, 1, 1)
	}
	else if (consider_deleted)
	{
		CONVERTER_BODY(item, 1, 1, 0)
	}
	else if (consider_filtered)
	{
		CONVERTER_BODY(item, 1, 0, 1)
	}
	else
		return item;
}

long StatisticWindow::PoresStatisticList::index_to_item(std::size_t index)
{
	bool consider_deleted = !parent_statwindow->settings_window->m_checkBox_deleted->IsChecked() && !Frame::frame->m_measure->m_deleted_pores.empty(),
		consider_filtered = !parent_statwindow->settings_window->m_checkBox_filtered->IsChecked() && !Frame::frame->m_measure->m_filtered_pores.empty();
	if (consider_deleted && consider_filtered)
	{
		CONVERTER_BODY(index, 0, 1, 1)
	}
	else if (consider_deleted)
	{
		CONVERTER_BODY(index, 0, 1, 0)
	}
	else if (consider_filtered)
	{
		CONVERTER_BODY(index, 0, 0, 1)
	}
	else
		return index;
}

wxString StatisticWindow::PoresStatisticList::OnGetItemText(long item, long column) const
{
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
			[[likely]] default:  *std::to_chars(buf, buf + 16, std::get<ID>(container[item_to_index(item)])).ptr = '\0'; return {buf};
			}
		}
#define CASE_NUMBER(r, x, value) case value: if (parent_statwindow->num_considered > 0 || item > 3) {*std::to_chars(buf, buf + 16, std::get<value>(container[item_to_index(item)])).ptr = '\0'; return {buf};} else return {'-'};
		BOOST_PP_SEQ_FOR_EACH(CASE_NUMBER, ~, PORES_CALCULATING_PARAMS)
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
	else [[likely]] if (auto find_it = attributes.find(std::get<ID>(container[item_to_index(item)])); find_it != attributes.end()) [[unlikely]]
		return const_cast<wxItemAttr*>(&find_it->second);
	else [[likely]]
		return wxListCtrl::OnGetItemAttr(item);
}

void StatisticWindow::PoresStatisticList::OnSelection(wxListEvent& event)
{
	if (dont_affect)
		return;
	long item = event.GetIndex();
	if (item > 3)
	{
		std::size_t index = item_to_index(item);
		uint32_t pore_id = std::get<ID>(container[index]);
		if (!Frame::frame->m_measure->m_deleted_pores.contains(pore_id) && !Frame::frame->m_measure->m_filtered_pores.contains(pore_id))
		{
			ImageWindow* image = Frame::frame->m_image;
			if (!image->m_sel_session.has_value())
				image->m_sel_session.emplace(image);
			image->m_sel_session.value().select(pore_id);
			float pore_x = std::get<CENTROID_X>(container[index]), pore_y = std::get<CENTROID_Y>(container[index]);
			image->scale_center.x = nearest_integer<int>(pore_x);
			image->scale_center.y = nearest_integer<int>(pore_y);
			image->Refresh();
			image->Update();
		}
	}
}

void StatisticWindow::PoresStatisticList::OnDeselection(wxListEvent& event)
{
	if (dont_affect)
		return;
	MeasureWindow* measure = Frame::frame->m_measure;
	std::vector<uint32_t> selected;
	selected.reserve(GetSelectedItemCount());
	auto item2index_converter = GET_CONVERTER(true);
	for (long item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); item != -1; item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED))
		selected.push_back(std::get<ID>(container[item2index_converter(item)]));
	bool do_refresh = false;
	for (auto it = measure->m_selected_pores.begin(), end = measure->m_selected_pores.end(); it != end;)
	{
		uint32_t id = *it++;
		if (auto find_it = std::lower_bound(selected.begin(), selected.end(), id); find_it == selected.end() || *find_it != id)
		{
			Frame::frame->m_image->m_sel_session.value().deselect(id);
			do_refresh = true;
		}
	}
	if (do_refresh)
	{
		if (measure->m_selected_pores.empty())
			Frame::frame->m_image->m_sel_session = std::nullopt;
		Frame::frame->m_image->Refresh();
		Frame::frame->m_image->Update();
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
	long item = index_to_item(ID_TO_INDEX(pore_id));
	SetItemState(item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	wxRect rect;
	GetItemRect(0, rect);
	ScrollList(0, rect.height*(item - GetScrollPos(wxVERTICAL)));
	dont_affect = false;
}

void StatisticWindow::PoresStatisticList::deselect_item(uint32_t pore_id)
{
	dont_affect = true;
	SetItemState(index_to_item(ID_TO_INDEX(pore_id)), 0, wxLIST_STATE_SELECTED);
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
	wxItemAttr attr;
	attr.SetBackgroundColour(0xAAAAEE);
	attributes.insert({pore_id, attr});
	row_t& item_row = container[ID_TO_INDEX(pore_id)];
	--parent_statwindow->num_considered;

#define CHECK_MIN_MAX(z, n, is_min) \
		{ \
			if (float& extreme_value = std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(container[BOOST_PP_IF(is_min, 2, 3)]); std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(item_row) BOOST_PP_IF(is_min, <=, >=) extreme_value) [[unlikely]] \
			{ \
				MeasureWindow::double_slider_t* slider = Frame::frame->m_measure->BOOST_PP_SEQ_ELEM(n, SLIDERS_ACTUAL); \
				float slider_new_extreme = std::numeric_limits<float>::BOOST_PP_IF(is_min, max, min)(); \
				extreme_value = std::numeric_limits<float>::BOOST_PP_IF(is_min, max, min)(); \
				for (auto it = container.begin() + 4, end = container.end(); it != end; ++it) \
				{ \
					if (float pore_value = std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it); pore_value BOOST_PP_IF(is_min, <, >) extreme_value && !Frame::frame->m_measure->m_deleted_pores.contains(std::get<ID>(*it))) [[unlikely]] \
					{ \
						if (!Frame::frame->m_measure->m_filtered_pores.contains(std::get<ID>(*it))) \
							extreme_value = pore_value; \
						if (pore_value BOOST_PP_IF(is_min, <, >) slider_new_extreme) \
							slider_new_extreme = pore_value; \
					} \
				} \
				if (slider->BOOST_PP_IF(is_min, min, max) BOOST_PP_IF(is_min, <, >) slider_new_extreme) \
				{ \
					slider->BOOST_PP_CAT(set_, BOOST_PP_IF(is_min, min, max))(slider_new_extreme); \
					slider->Refresh(); \
					slider->Update(); \
				} \
			} \
		}
	RECALCULATE(1)
#undef CHECK_MIN_MAX

	after_changes(std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[4]) -= Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_multiset>().count(pore_id));
}

void StatisticWindow::PoresStatisticList::on_pore_recovered(uint32_t pore_id)
{
	row_t& item_row = container[ID_TO_INDEX(pore_id)];

	bool filtered = false;
#define CHECK_FILTERS(z, _, i, s) \
		{ \
			float value = std::get<BOOST_PP_SEQ_ELEM(i, PORES_CALCULATING_PARAMS)>(item_row); \
			bool refresh_max = Frame::frame->m_measure->s->max < value; \
			bool refresh_min = Frame::frame->m_measure->s->min > value; \
			if (refresh_max) [[unlikely]] \
				Frame::frame->m_measure->s->set_max(value); \
			if (refresh_min) [[unlikely]] \
				Frame::frame->m_measure->s->set_min(value); \
			if (refresh_min || refresh_max) \
			{ \
				Frame::frame->m_measure->s->Refresh(); \
				Frame::frame->m_measure->s->Update(); \
			} \
			if (auto [min_value, max_value] = Frame::frame->m_measure->s->get_values(); min_value > value || max_value < value) [[unlikely]] \
				filtered = true; \
		}
	BOOST_PP_SEQ_FOR_EACH_I(CHECK_FILTERS, ~, SLIDERS)
	if (filtered)
	{
		Frame::frame->m_measure->m_filtered_pores.insert(pore_id);
		attributes[pore_id].SetBackgroundColour(0xAAEEEE);
		Refresh();
		Update();
		return;
	}

	attributes.erase(pore_id);
	++parent_statwindow->num_considered;

#define CHECK_MIN_MAX(z, n, is_min) \
		{ \
			auto& extreme_value = std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(container[BOOST_PP_IF(is_min, 2, 3)]); \
			if (float item_value = std::get<BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(item_row); item_value BOOST_PP_IF(is_min, <, >) extreme_value) [[unlikely]] \
			{ \
				extreme_value = item_value; \
				if (item_value BOOST_PP_IF(is_min, <, >) Frame::frame->m_measure->BOOST_PP_SEQ_ELEM(n, SLIDERS_ACTUAL)->BOOST_PP_IF(is_min, min, max)) [[unlikely]] \
				{ \
					Frame::frame->m_measure->BOOST_PP_SEQ_ELEM(n, SLIDERS_ACTUAL)->BOOST_PP_CAT(set_, BOOST_PP_IF(is_min, min, max))(item_value); \
					Frame::frame->m_measure->BOOST_PP_SEQ_ELEM(n, SLIDERS_ACTUAL)->Refresh(); \
					Frame::frame->m_measure->BOOST_PP_SEQ_ELEM(n, SLIDERS_ACTUAL)->Update(); \
				} \
			} \
		}
	RECALCULATE(0)
#undef CHECK_MIN_MAX

	after_changes(std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[4]) += Frame::frame->m_measure->m_pores.get<MeasureWindow::tag_multiset>().count(pore_id));
}

void StatisticWindow::PoresStatisticList::set_item_count()
{
	uint32_t item_count = 4 + parent_statwindow->num_considered;
	if (parent_statwindow->settings_window->m_checkBox_deleted->IsChecked())
		item_count += Frame::frame->m_measure->m_deleted_pores.size();
	if (parent_statwindow->settings_window->m_checkBox_filtered->IsChecked())
		item_count += Frame::frame->m_measure->m_filtered_pores.size();
	SetItemCount(item_count);
}

void StatisticWindow::PoresStatisticList::set_columns_width()
{
#define SET_COLUMN_WIDTH(r, x, value) SetColumnWidth(value, wxLIST_AUTOSIZE);
	BOOST_PP_SEQ_FOR_EACH(SET_COLUMN_WIDTH, ~, PORES_CALCULATING_PARAMS)
}

void StatisticWindow::PoresStatisticList::after_changes(float pores_square)
{
	uint32_t all_size = Frame::frame->m_measure->width*Frame::frame->m_measure->height;
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[2]) = parent_statwindow->num_considered;
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[3]) = pores_square/float(all_size);
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[4]) = pores_square;
	std::get<CommonStatisticList::PARAM_VALUE>(parent_statwindow->common_statistic_list->container[5]) = all_size - pores_square;

	set_item_count();
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
	StatisticWindow::SettingsWindow::checkBox_deleted_id = wxWindow::NewControlId(),
	StatisticWindow::SettingsWindow::checkBox_filtered_id = wxWindow::NewControlId();

wxBEGIN_EVENT_TABLE(StatisticWindow::SettingsWindow, wxWindow)
	EVT_CHECKBOX(StatisticWindow::SettingsWindow::checkBox_color_id, StatisticWindow::SettingsWindow::OnDistributionColor)
	EVT_CHECKBOX(StatisticWindow::SettingsWindow::checkBox_deleted_id, StatisticWindow::SettingsWindow::OnShowDeleted)
	EVT_CHECKBOX(StatisticWindow::SettingsWindow::checkBox_filtered_id, StatisticWindow::SettingsWindow::OnShowFiltered)
wxEND_EVENT_TABLE()

StatisticWindow::SettingsWindow::SettingsWindow(wxWindow* parent) : wxWindow(parent, wxID_ANY), parent_statwindow(static_cast<StatisticWindow*>(parent))
{
	SetBackgroundColour(*wxLIGHT_GREY);

	wxStaticBoxSizer* static_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("Настройка")), wxVERTICAL);

	m_checkBox_color = new wxCheckBox(this, checkBox_color_id, wxT("Раскраска пор под цвет интервалов распределения"));
	static_box_sizer->Add( m_checkBox_color, 0, wxALL, 5 );

	m_checkBox_deleted = new wxCheckBox(this, checkBox_deleted_id, wxT("Показывать в списке удалённые поры"));
	static_box_sizer->Add( m_checkBox_deleted, 0, wxALL, 5 );

	m_checkBox_filtered = new wxCheckBox(this, checkBox_filtered_id, wxT("Показывать в списке отфильтрованные поры"));
	static_box_sizer->Add( m_checkBox_filtered, 0, wxALL, 5 );

	SetSizer(static_box_sizer);
}

void StatisticWindow::SettingsWindow::OnDistributionColor(wxCommandEvent& event)
{
	;
}

void StatisticWindow::SettingsWindow::OnShowDeleted(wxCommandEvent& event)
{
	parent_statwindow->pores_statistic_list->set_item_count();
	parent_statwindow->pores_statistic_list->Refresh();
	parent_statwindow->pores_statistic_list->Update();
}

void StatisticWindow::SettingsWindow::OnShowFiltered(wxCommandEvent& event)
{
	parent_statwindow->pores_statistic_list->set_item_count();
	parent_statwindow->pores_statistic_list->Refresh();
	parent_statwindow->pores_statistic_list->Update();
}