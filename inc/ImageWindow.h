#pragma once

#include <wx/window.h>
#include <wx/image.h>
//#include <boost/gil.hpp>

//namespace gil = boost::gil;

class ImageWindow : public wxWindow
{
public:
	static wxImage m_img_original, m_img_edited;
	static void ToGrayscale();
	enum class EStatus {COLOR, GREY, BINARY};
	static EStatus status;

	ImageWindow(wxWindow *parent);
	void OnPaint(wxPaintEvent& event);

	wxDECLARE_EVENT_TABLE();
};