#include "StatisticWindow.h"
#include "Frame.h"
#include <wx/dcbuffer.h>
#include <charconv>

// StatisticWindow

wxBEGIN_EVENT_TABLE(StatisticWindow, wxWindow)
	EVT_PAINT(StatisticWindow::OnPaint)
wxEND_EVENT_TABLE()

StatisticWindow::StatisticWindow(wxWindow* parent)
	: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	
	m_aui = new Aui{this};

	pores_statistic_list = new StatisticList<PoresStatistic>(this);
	pores_statistic_list->AppendColumn(wxT("№"));
	pores_statistic_list->AppendColumn(wxT("Площадь"));
	pores_statistic_list->AppendColumn(wxT("Периметр"));
	common_statistic_list = new StatisticList<CommonStatistic>(this);
	common_statistic_list ->AppendColumn(wxT("Параметр"));
	common_statistic_list ->AppendColumn(wxT("Значение"));
	m_aui->AddPane(common_statistic_list, wxAuiPaneInfo{}.Caption(wxT("Общая характеристика")).CloseButton(false).Top().MaximizeButton().PaneBorder(true).Name(wxT("Common"))/*.MinSize(66, 66)*/);
	m_aui->AddPane(pores_statistic_list, wxAuiPaneInfo{}.Caption(wxT("Характеристики пор")).CloseButton(false).Top().MaximizeButton().PaneBorder(true).Name(wxT("Pores")));

	settings_window = new wxPanel(this);
	settings_window->SetBackgroundColour(*wxLIGHT_GREY);
	m_aui->AddPane(settings_window, wxAuiPaneInfo{}.CenterPane().Name(wxT("Settings")));

	distribution_window = new DistributionWindow(this);
	m_aui->AddPane(distribution_window, wxAuiPaneInfo{}.Caption(wxT("Распределение")).CloseButton(false).Right().MaximizeButton().PaneBorder(true).Name(wxT("Distrib")));

	m_aui->Update();
}

void StatisticWindow::CollectStatistic()
{
	MeasureWindow* measure = static_cast<Frame*>(GetParent())->m_measure;
	uint32_t pores_num = measure->pores_count - measure->m_deleted_pores.size(), pores_square = 0;
	common_statistic_list->SetItemCount(4);
	common_statistic_list->container[0] = CommonStatistic::row_t{wxString{"Количество пор"}, pores_num};
	pores_statistic_list->container.clear();
	pores_statistic_list->container.reserve(pores_num);
	pores_statistic_list->SetItemCount(pores_num);
	auto finalize = [this, &pores_square]()
	{
		uint32_t all_size = GetSize().GetWidth()*GetSize().GetHeight();
		common_statistic_list->container[1] = CommonStatistic::row_t{wxString{"Удельное количество пор"}, pores_square/float(all_size)};
		common_statistic_list->container[2] = CommonStatistic::row_t{wxString{"Площадь пор"}, pores_square};
		common_statistic_list->container[3] = CommonStatistic::row_t{wxString{"Площадь матрицы"}, all_size - pores_square};
		common_statistic_list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
		common_statistic_list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
		for (uint8_t i = 0, count = pores_statistic_list->GetColumnCount(); i < count; ++i)
			pores_statistic_list->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);
		common_statistic_list->Refresh();
		pores_statistic_list->Refresh();
		common_statistic_list->Update();
		pores_statistic_list->Update();
	};

	auto it = measure->m_pores.get<MeasureWindow::tag_multiset>().begin(), end = measure->m_pores.get<MeasureWindow::tag_multiset>().end();
	for (uint32_t id = it->first, square = 0, perimeter = 0;; id = it->first, pores_square += square, square = 0, perimeter = 0)
	{
		if (!measure->m_deleted_pores.contains(id))
		{
			do
			{
				++square;
				if (measure->m_boundary_pixels.contains({it->second.first, it->second.second})) [[unlikely]]
					++perimeter;
				if (++it == end) [[unlikely]]
				{
					pores_statistic_list->container.emplace_back(id, square, perimeter);
					finalize();
					return;
				}
			} while (it->first == id);
			pores_statistic_list->container.emplace_back(id, square, perimeter);
		}
		else if ((it = measure->m_pores.get<MeasureWindow::tag_multiset>().lower_bound(id, [](uint32_t lhs, uint32_t rhs) {return lhs <= rhs;}))
			== measure->m_pores.get<MeasureWindow::tag_multiset>().end()) [[unlikely]]
		{
			finalize();
			return;
		}
	}
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

// StatisticList

template <class Kind>
StatisticWindow::StatisticList<Kind>::StatisticList(wxWindow* parent)
	: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL | wxLC_HRULES | wxLC_VRULES)
{
	EnableAlternateRowColours();
}

template <class Kind>
wxString StatisticWindow::StatisticList<Kind>::OnGetItemText(long item, long column) const
{
	char buf[16];
#define CASE_NUMBER(value) case Kind::value: *std::to_chars(buf, buf + 16, std::get<Kind::value>(Kind::container[item])).ptr = '\0'; return {buf};

	if constexpr (std::is_same_v<Kind, CommonStatistic>)
		switch (column)
		{
		case Kind::PARAM_NAME: return std::get<Kind::PARAM_NAME>(Kind::container[item]);
		CASE_NUMBER(PARAM_VALUE)
		}
	else if constexpr (std::is_same_v<Kind, PoresStatistic>)
		switch (column)
		{
		CASE_NUMBER(ID)
		CASE_NUMBER(SQUARE)
		CASE_NUMBER(PERIMETER)
		}
	__assume(false);
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