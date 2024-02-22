#pragma once

#include <wx/window.h>
#include <wx/listctrl.h>
#include <wx/checkbox.h>
#include <wx/aui/aui.h>
#include <array>

class Frame;

class StatisticWindow : public wxWindow
{
	struct Aui : wxAuiManager
	{
		Aui(wxWindow* parent) : wxAuiManager(parent, wxAUI_MGR_TRANSPARENT_HINT | wxAUI_MGR_HINT_FADE | wxAUI_MGR_NO_VENETIAN_BLINDS_FADE | wxAUI_MGR_LIVE_RESIZE) {}
        void InitPanesPositions(wxSize size);
	};

	struct CommonStatisticList : wxListCtrl
	{
		using row_t = std::tuple<wxString, float>;
		enum : uint8_t {PARAM_NAME, PARAM_VALUE};
		std::array<row_t, 10> container;

		CommonStatisticList(wxWindow* parent);
		wxString OnGetItemText(long item, long column) const override;
	};

	struct PoresStatisticList : wxListCtrl
	{
		using row_t = std::tuple<uint32_t, float, float, float, std::pair<float, float>, float, float, float, float, float, float, float, float>;
		enum : uint8_t {ID, SQUARE, PERIMETER, DIAMETER, CENTROID, LENGTH, WIDTH, LENGTH_OY, WIDTH_OX, MAX_DIAMETER, MIN_DIAMETER, SHAPE, ELONGATION};
		std::vector<row_t> container;
		std::unordered_map<uint32_t, wxItemAttr> attributes;

		PoresStatisticList(wxWindow* parent);
		wxString OnGetItemText(long item, long column) const override;
		wxItemAttr* OnGetItemAttr(long item) const override;
		void OnSelection(wxListEvent& event);
		void OnDeselection(wxListEvent& event);

	private:
		wxDECLARE_EVENT_TABLE();
	};

	struct DistributionWindow : wxWindow
	{
		DistributionWindow(wxWindow* parent);
		void OnPaint(wxPaintEvent& event);

	private:
		wxDECLARE_EVENT_TABLE();
	};

	struct SettingsWindow : wxWindow
	{
		wxCheckBox *m_checkBox_color, *m_checkBox_deleted;
		static wxWindowID checkBox_color_id, checkBox_deleted_id;

		SettingsWindow(wxWindow* parent);
		void OnDistributionColor(wxCommandEvent& event);
		void OnShowDeleted(wxCommandEvent& event);

	private:
		StatisticWindow* parent_statwindow;

		wxDECLARE_EVENT_TABLE();
	};

	wxDECLARE_EVENT_TABLE();

public:
	Aui* m_aui;
	CommonStatisticList* common_statistic_list;
	PoresStatisticList* pores_statistic_list;
	DistributionWindow* distribution_window;
	SettingsWindow* settings_window;
	Frame* parent_frame;

	StatisticWindow(wxWindow* parent);
	~StatisticWindow() {delete m_aui;}
	void CollectStatistic();
	void OnPaint(wxPaintEvent& event);
};