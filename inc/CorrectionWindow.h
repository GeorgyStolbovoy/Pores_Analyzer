#pragma once

#include <wx/window.h>
#include <wx/geometry.h>
#include <wx/graphics.h>
#include <optional>
#include <set>
#include <variant>
#include <thread>

class CorrectionWindow : public wxWindow
{
	using OptionalPath = std::optional<wxGraphicsPath>;
	struct PointComplex
	{
		wxPoint2DDouble point;
		bool hovered, edge;
		
		PointComplex(wxPoint2DDouble p, bool h, bool e) : point(p), hovered(h), edge(e) {}
	};
	struct CurveComplex
	{
		using CurvePoints = std::pair<const wxPoint2DDouble*, const wxPoint2DDouble*>;
		using ControllerPoint = std::pair<wxPoint2DDouble, bool>;

		CurvePoints points;
		ControllerPoint ctrl_point;
		float slope;
		OptionalPath path = std::nullopt;
		
		CurveComplex(CurvePoints&& ps, ControllerPoint&& c_p) : points(std::move(ps)), ctrl_point(std::move(c_p)), slope(0.0f) {}
		CurveComplex(CurvePoints&& ps, CurveComplex* other) :
			points(std::move(ps)),
			ctrl_point([this, other](){double x = (points.second->m_x + points.first->m_x)/2; return ControllerPoint{{x, CalcY(x, other->slope, other->points)}, false};}()),
			slope(CalcSlope())
		{}
		double CalcY(double x) const
		{ 
			return (points.second->m_y - points.first->m_y)*std::pow((x - points.first->m_x)/(points.second->m_x - points.first->m_x), std::pow(10, slope)) + points.first->m_y;
		}
		double CalcY(double x, float other_slope, CurvePoints& other_points) const
		{ 
			return (other_points.second->m_y - other_points.first->m_y)*std::pow((x - other_points.first->m_x)/(other_points.second->m_x - other_points.first->m_x), std::pow(10, other_slope)) + other_points.first->m_y;
		}
		float CalcSlope() const
		{
			return std::log10(
				std::log10((ctrl_point.first.m_y - points.first->m_y)/(points.second->m_y - points.first->m_y)) /
				std::log10((ctrl_point.first.m_x - points.first->m_x)/(points.second->m_x - points.first->m_x))
			);
		}
		void CalcCtrlPoint()
		{
			ctrl_point.first.m_x = (points.second->m_x + points.first->m_x)/2;
			ctrl_point.first.m_y = (points.second->m_y - points.first->m_y) *
				std::pow((ctrl_point.first.m_x - points.first->m_x)/(points.second->m_x - points.first->m_x), std::pow(10, slope)) + points.first->m_y;
		}
	};
	struct CurvesComp
	{
		using is_transparent = void;
		bool operator()(const CurveComplex& lhs, const CurveComplex& rhs) const {return lhs.points.first->m_x < rhs.points.first->m_x;}
		bool operator()(const double x, const CurveComplex& complex) const {return x < complex.points.first->m_x;}
		bool operator()(const CurveComplex& complex, const double x) const {return x > complex.points.first->m_x;}
	};

	int saved_width, saved_height;
	std::set<PointComplex, decltype([](const PointComplex& lhs, const PointComplex& rhs){return lhs.point.m_x < rhs.point.m_x;})> points;
	std::set<CurveComplex, CurvesComp> curves;
	std::optional<wxPoint2DDouble> point_to_add;
	std::variant<std::monostate, PointComplex*, CurveComplex*> hover, captured;
	bool ctrl_pressed = false, alt_pressed = false; // antispam

	double ConvertMouseY(int y) {int h = GetSize().GetHeight(); return -y*(13 + h)/h + h + 6.5;}
	double ConvertMouseX(int x) {int w = GetSize().GetWidth(); return x*(13 + w)/w - 6.5;}
	void ResetHover();
	template <bool do_refresh> void ProcessHover(int mouse_x, int mouse_y);
	void ProcessAdding(int mouse_x, int mouse_y);
	void UpdateImage();

public:
	OptionalPath path_histogram;

	CorrectionWindow(wxWindow *parent, const wxSize& size);
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnMouseEnter(wxMouseEvent& event) {SetFocus();}
	void OnMouseMove(wxMouseEvent& event);
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseLeftUp(wxMouseEvent& event);
	void OnCaptureLost(wxMouseCaptureLostEvent& event) {captured = std::monostate{};}

	wxDECLARE_EVENT_TABLE();
};