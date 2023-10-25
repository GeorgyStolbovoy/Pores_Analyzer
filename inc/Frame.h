#pragma once

#include "ImageWindow.h"
#include "CorrectionWindow.h"
#include <wx/frame.h>
#include <wx/statline.h>
#include <wx/slider.h>
#include <wx/checkbox.h>

class Frame : public wxFrame
{
    ImageWindow* m_image;
	CorrectionWindow* m_curves;

	wxStaticLine* m_staticline1;
	wxSlider* m_slider2;
	wxStaticLine* m_staticline2;
	wxCheckBox* m_checkBox1;
	wxSlider* m_slider1;

public:
    Frame();
	void RefreshImage() {m_image->Refresh();}
};