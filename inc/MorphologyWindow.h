#pragma once

#include <wx/window.h>
#include <wx/dialog.h>
#include <wx/dcclient.h>
#include <boost/container_hash/hash.hpp>
#include <optional>
#include <unordered_set>

template <template <class... T> class S, class... Ys>
struct StructureHolder
{
	using structure_element = std::pair<int8_t, int8_t>;
	using structure_t = S<structure_element, Ys...>;

	structure_t structure;
	int8_t max_ofs = 1;

	StructureHolder(structure_t&& structure) : structure(std::move(structure)) {}
	int8_t calc_offset();
	std::pair<double, double> calc_offset_and_get_lines_cells_count(int8_t default_ofs = 1);
	void display(wxGraphicsContext* gc, int width, int height, int line_width, double line_count, double cells_count);
};

struct MorphologyWindow : wxWindow, StructureHolder<std::vector>
{
	MorphologyWindow(wxWindow *parent);
	void OnPaint(wxPaintEvent& event);
	void OnMouseLeftUp(wxMouseEvent& event);

private:
	class MorphologyDialog : public wxDialog
	{
		struct StructureEditingWindow : wxWindow, StructureHolder<std::unordered_set, boost::hash<structure_element>>
		{
			int8_t current_ofs = 1;

			StructureEditingWindow(wxWindow *parent, structure_t&& structure);
			void OnPaint(wxPaintEvent& event);
			void OnMouseMove(wxMouseEvent& event);
			void OnMouseLeftDown(wxMouseEvent& event);
			void OnMouseLeftUp(wxMouseEvent& event);
			void OnMouseRightDown(wxMouseEvent& event);
			void OnMouseRightUp(wxMouseEvent& event);
			void OnMouseWheel(wxMouseEvent& event);
			void OnLeave(wxMouseEvent& event);

		private:
			std::optional<structure_element> hover;
			int line_width;
			bool add_or_remove_mode;

			void calc_params(double& cell_width, double& cell_height);
			void calc_params(double& cell_width, double& cell_height, int8_t& ofs_x, int8_t& ofs_y, int x, int y);

			wxDECLARE_EVENT_TABLE();
		} *m_struct_window;
		wxButton *m_button_reset, *m_button_ok, *m_button_cancel;

		wxDECLARE_EVENT_TABLE();

	public:
		static wxWindowIDRef button_reset_id, button_ok_id, button_cancel_id;

		MorphologyDialog(wxWindow* parent);
		void OnReset(wxCommandEvent& event);
		void OnOk(wxCommandEvent& event);
		void OnCancel(wxCommandEvent& event);
		void Exit(wxCloseEvent& event);
	};

	wxDECLARE_EVENT_TABLE();
};