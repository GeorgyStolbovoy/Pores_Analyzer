#pragma once

#include <wx/window.h>
#include <wx/listctrl.h>
#include <wx/aui/aui.h>
#include <array>

class StatisticWindow : public wxWindow
{
	struct Aui : wxAuiManager
	{
		Aui(wxWindow* parent) : wxAuiManager(parent, wxAUI_MGR_TRANSPARENT_HINT | wxAUI_MGR_HINT_FADE | wxAUI_MGR_NO_VENETIAN_BLINDS_FADE | wxAUI_MGR_LIVE_RESIZE) {}
        void InitPanesPositions(wxSize size);
	};

	struct CommonStatistic
	{
		using row_t = std::tuple<wxString, float>;
		enum : uint8_t {PARAM_NAME, PARAM_VALUE};
		std::array<row_t, 10> container;
	};
	struct PoresStatistic
	{
		using row_t = std::tuple<uint32_t, float, float>;
		enum : uint8_t {ID, SQUARE, PERIMETER};
		std::vector<row_t> container;
	};
	template <class Kind>
	struct StatisticList : wxListCtrl, Kind
	{
		StatisticList(wxWindow* parent);
		wxString OnGetItemText(long item, long column) const override;
	};

	struct DistributionWindow : wxWindow
	{
		DistributionWindow(wxWindow* parent);
		void OnPaint(wxPaintEvent& event);

	private:
		wxDECLARE_EVENT_TABLE();
	};

public:
	Aui* m_aui;
	StatisticList<CommonStatistic>* common_statistic_list;
	StatisticList<PoresStatistic>* pores_statistic_list;
	DistributionWindow* distribution_window;
	wxPanel* settings_window;

	StatisticWindow(wxWindow* parent);
	~StatisticWindow() {delete m_aui;}
	void CollectStatistic();
	void OnPaint(wxPaintEvent& event);

private:
	wxDECLARE_EVENT_TABLE();
};