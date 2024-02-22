#include "StatisticWindow.h"
#include "Frame.h"
#include <wx/dcbuffer.h>
#include <wx/statbox.h>
#include <charconv>
#include <boost/preprocessor.hpp>

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
	struct statistic_collector 
	{
		StatisticWindow* self;
		MeasureWindow* measure;
		bool collect_deleted;
		uint32_t pores_num, pores_square = 0;
		double perimeter_mean = 0, diameter_mean = 0, sum_x_mean = 0, sum_y_mean = 0, lenght_oy_mean = 0, width_ox_mean = 0, shape_mean = 0, elongation_mean = 0;

		statistic_collector(StatisticWindow* self) :
			self(self), measure(self->parent_frame->m_measure), collect_deleted(self->settings_window->m_checkBox_deleted->IsChecked()), pores_num(collect_deleted ? measure->pores_count : measure->pores_count - measure->m_deleted_pores.size())
		{
			self->common_statistic_list->SetItemCount(6);
			self->pores_statistic_list->attributes.clear();
			self->pores_statistic_list->container.clear();
			self->pores_statistic_list->container.reserve(pores_num + 4);
			self->pores_statistic_list->container.resize(2, {0, 0, 0, 0, {0, 0}, 0, 0, 0, 0, 0, 0, 0, 0});
			constexpr float fmax = std::numeric_limits<float>::max();
			self->pores_statistic_list->container.push_back({0, fmax, fmax, fmax, {fmax, fmax}, fmax, fmax, fmax, fmax, fmax, fmax, fmax, fmax});
			self->pores_statistic_list->container.push_back({0, 0, 0, 0, {0, 0}, 0, 0, 0, 0, 0, 0, 0, 0});
			self->pores_statistic_list->SetItemCount(pores_num + 4);

			auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().begin(), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
			MeasureWindow::pores_container::index<MeasureWindow::tag_hashset>::type::iterator end_it = measure->m_pores.get<MeasureWindow::tag_hashset>().end(), tmp_it;
			for (uint32_t id = it->first, square = 0, perimeter = 0, sum_x = 0, sum_y = 0, min_x = measure->width - 1, max_x = 0, min_y = measure->height - 1, max_y = 0;;
				id = it->first, square = 0, perimeter = 0, sum_x = 0, sum_y = 0, min_x = measure->width - 1, max_x = 0, min_y = measure->height - 1, max_y = 0)
			{
				if (bool not_deleted = !measure->m_deleted_pores.contains(id); not_deleted || collect_deleted)
				{
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
#define INC_IF_BOUNDARY(dx, dy) if ((tmp_it = measure->m_pores.get<MeasureWindow::tag_hashset>().find(MeasureWindow::coord_t{it->second.first dx, it->second.second dy})) == end_it || tmp_it->first != id) ++perimeter
							INC_IF_BOUNDARY(-1,);
							INC_IF_BOUNDARY(,+1);
							INC_IF_BOUNDARY(+1,);
							INC_IF_BOUNDARY(,-1);
						}
						if (++it == end) [[unlikely]]
						{
							collect_for_one(id, square, perimeter, {sum_x/float(square), sum_y/float(square)}, max_y - min_y + 1, max_x - min_x + 1, not_deleted);
							return;
						}
					} while (it->first == id);
					collect_for_one(id, square, perimeter, {sum_x/float(square), sum_y/float(square)}, max_y - min_y + 1, max_x - min_x + 1, not_deleted);
				}
				else if ((it = measure->m_pores.get<MeasureWindow::tag_multiset>().lower_bound(id, [](uint32_t lhs, uint32_t rhs) {return lhs <= rhs;})) == end) [[unlikely]]
					return;
			}
		}
		void collect_for_one(uint32_t id, uint32_t square, uint32_t perimeter, std::pair<float, float> centroid, uint32_t lenght_oy, uint32_t width_ox, bool consider)
		{
			float shape = 4*M_PI*square/(perimeter*perimeter), diameter = std::sqrtf(4*square/M_PI), elongation = 1/shape;
			self->pores_statistic_list->container.emplace_back(id, square, perimeter, diameter, centroid, PoresStatisticList::LENGTH, PoresStatisticList::WIDTH, lenght_oy, width_ox, PoresStatisticList::MAX_DIAMETER, PoresStatisticList::MIN_DIAMETER, shape, elongation);
			
			if (consider)
			{
				pores_square += square,
				perimeter_mean += perimeter;
				diameter_mean += diameter;
				sum_x_mean += centroid.first;
				sum_y_mean += centroid.second;
				lenght_oy_mean += lenght_oy;
				width_ox_mean += width_ox;
				shape_mean += shape;
				elongation_mean += elongation;

#define PARAMS_NAMES (SQUARE)(PERIMETER)(DIAMETER)(CENTROID)(LENGTH_OY)(WIDTH_OX)(SHAPE)(ELONGATION)
#define PARAMS_VAARGS (square),(perimeter),(diameter),(centroid.first, centroid.second),(lenght_oy),(width_ox),(shape),(elongation)
#define PARAMS BOOST_PP_VARIADIC_TO_SEQ(PARAMS_VAARGS)
#define SET_MIN_MAX_IMPL(value, param, op) \
				if (value op param) [[unlikely]] \
					value = param;
#define SET_MIN_MAX(z, n, flag) \
				{ \
					auto& value = std::get<PoresStatistic::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(self->pores_statistic_list->container[flag]); \
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
				self->pores_statistic_list->attributes.insert({id, attr});
			}
		}
		~statistic_collector()
		{
			uint32_t all_size = measure->width*measure->height;
			self->common_statistic_list->container[0] = CommonStatisticList::row_t{wxString{"Ширина"}, measure->width};
			self->common_statistic_list->container[1] = CommonStatisticList::row_t{wxString{"Высота"}, measure->height};
			self->common_statistic_list->container[2] = CommonStatisticList::row_t{wxString{"Количество пор"}, pores_num};
			self->common_statistic_list->container[3] = CommonStatisticList::row_t{wxString{"Удельное количество пор"}, pores_square/float(all_size)};
			self->common_statistic_list->container[4] = CommonStatisticList::row_t{wxString{"Площадь пор"}, pores_square};
			self->common_statistic_list->container[5] = CommonStatisticList::row_t{wxString{"Площадь матрицы"}, all_size - pores_square};

#define PARAMS_VAARGS (pores_square),(perimeter_mean),(diameter_mean),(sum_x_mean, sum_y_mean),(lenght_oy_mean),(width_ox_mean),(shape_mean),(elongation_mean)
#define PARAMS BOOST_PP_VARIADIC_TO_SEQ(PARAMS_VAARGS)
#define SET_MEAN_DEVIATION(z, n, __) \
			{ \
				auto& mean = std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(self->pores_statistic_list->container[0]); \
				BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), \
					mean = BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(n, PARAMS))/float(pores_num);, \
					mean.first = BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(n, PARAMS))/pores_num; \
					mean.second = BOOST_PP_TUPLE_ELEM(1, BOOST_PP_SEQ_ELEM(n, PARAMS))/pores_num;) \
				BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), double deviation = 0;, double deviation_x = 0; double deviation_y = 0;) \
				for (auto it = self->pores_statistic_list->container.begin() + 4, end = self->pores_statistic_list->container.end(); it != end; ++it) \
					if (!self->pores_statistic_list->attributes.contains(std::get<PoresStatisticList::ID>(*it))) \
					BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), \
						deviation += std::pow(mean - std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it), 2);, \
						{ \
							deviation_x += std::pow(mean.first - std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it).first, 2); \
							deviation_y += std::pow(mean.second - std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(*it).second, 2); \
						}) \
				std::get<PoresStatisticList::BOOST_PP_SEQ_ELEM(n, PARAMS_NAMES)>(self->pores_statistic_list->container[1]) = \
					BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(n, PARAMS)), 1), std::sqrtf(deviation/(pores_num-1));, (std::pair{std::sqrtf(deviation_x/(pores_num-1)), std::sqrtf(deviation_y/(pores_num-1))});) \
			}
			BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(PARAMS), SET_MEAN_DEVIATION, ~)
#undef PARAMS_VAARGS
#undef PARAMS

			self->common_statistic_list->SetColumnWidth(0, wxLIST_AUTOSIZE);
			self->common_statistic_list->SetColumnWidth(1, wxLIST_AUTOSIZE);
			for (uint8_t i = 0, count = self->pores_statistic_list->GetColumnCount(); i < count; ++i)
				self->pores_statistic_list->SetColumnWidth(i, wxLIST_AUTOSIZE);
			self->common_statistic_list->Refresh();
			self->pores_statistic_list->Refresh();
			self->common_statistic_list->Update();
			self->pores_statistic_list->Update();
		}
	} __{this};	
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

StatisticWindow::PoresStatisticList::PoresStatisticList(wxWindow* parent)
	: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL | wxLC_HRULES | wxLC_VRULES)
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
	ImageWindow::SelectionSession& sel = parent_frame;
}

void StatisticWindow::PoresStatisticList::OnDeselection(wxListEvent& event)
{
	;
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
		MeasureWindow::pores_container::index<MeasureWindow::tag_hashset>::type::iterator tmp_it, end_it = measure->m_pores.get<MeasureWindow::tag_hashset>().end();
		MeasureWindow::pores_container::index<MeasureWindow::tag_multiset>::type::iterator it, end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
		for (uint32_t id : measure->m_deleted_pores)
		{
			it = measure->m_pores.get<MeasureWindow::tag_multiset>().find(id);
			uint32_t square = 0, perimeter = 0, sum_x = 0, sum_y = 0, min_x = measure->width - 1, max_x = 0, min_y = measure->height - 1, max_y = 0;
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
#define INC_IF_BOUNDARY(dx, dy) if ((tmp_it = measure->m_pores.get<MeasureWindow::tag_hashset>().find(MeasureWindow::coord_t{it->second.first dx, it->second.second dy})) == end_it || tmp_it->first != id) ++perimeter
					INC_IF_BOUNDARY(-1,);
					INC_IF_BOUNDARY(,+1);
					INC_IF_BOUNDARY(+1,);
					INC_IF_BOUNDARY(,-1);
				}
				if (++it == end) [[unlikely]]
					break;
			} while (it->first == id);
			float shape = 4*M_PI*square/(perimeter*perimeter), diameter = std::sqrtf(4*square/M_PI), elongation = 1/shape;
			parent_statwindow->pores_statistic_list->container.emplace_back(id, square, perimeter, diameter, std::pair{sum_x/float(square), sum_y/float(square)},
				PoresStatisticList::LENGTH, PoresStatisticList::WIDTH, max_y - min_y + 1, max_x - min_x + 1, PoresStatisticList::MAX_DIAMETER, PoresStatisticList::MIN_DIAMETER, shape, elongation);

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
	for (uint8_t i = 0, count = parent_statwindow->pores_statistic_list->GetColumnCount(); i < count; ++i)
		parent_statwindow->pores_statistic_list->SetColumnWidth(i, wxLIST_AUTOSIZE);
	parent_statwindow->pores_statistic_list->Refresh();
	parent_statwindow->pores_statistic_list->Update();
}