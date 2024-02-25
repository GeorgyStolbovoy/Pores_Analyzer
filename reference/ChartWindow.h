#pragma once

#include "framework.h"
#include "WinAPI.h"
#include "BaseWindow.h"
#include "d2d1.h"
#include <dwrite.h>
#include <vector>
#include <string>

class ChartWindow : public BaseWindow<ChartWindow>
{
public:
	ChartWindow();
	~ChartWindow();
	PCWSTR  ClassName() const override { return szWindowClass; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	//HRESULT DrawChart(std::vector<D2D1_POINT_2F>& collection, unsigned char vert_lines_pairs = 5, unsigned char horis_lines_pairs = 5);
	HRESULT DrawHistogram();
	HRESULT CreatePathTemplate();

	ID2D1Factory* pFactory;
	ID2D1HwndRenderTarget* pRenderTarget;
	ID2D1PathGeometry *pPathAxis, *pPathRects[3], *pPathLines[3], *pPathFunc[3], *pPathNotches[3];
	ID2D1LinearGradientBrush* pGBrush[3];
	ID2D1SolidColorBrush* pSBrush;
	ID2D1StrokeStyle* pStroke;
	IDWriteFactory* pDWFactory;
	IDWriteTextFormat *pTextFormat_1, *pTextFormat_2;
	D2D1_SIZE_F size;
	float scale;
	float offset{ 50.0f }, notch{ 10.0f }, axis_offset{ 30.0f };
	std::vector<float>* pValues;
	std::vector< std::tuple<D2D1_RECT_F, D2D1_COLOR_F, std::pair<DWRITE_TEXT_ALIGNMENT, DWRITE_PARAGRAPH_ALIGNMENT>, float> > stored_text[3];
	std::vector< std::pair<D2D1_RECT_F, float> > common_text;
	std::vector< std::pair<D2D1_RECT_F, std::wstring> > legenda[2];
	bool need_redraw[3]{ false, false, false };

private:
	D2D1_GRADIENT_STOP gradientStops[3]{ {0.0f, D2D1::ColorF(D2D1::ColorF::Blue, 1)}, {0.5f, D2D1::ColorF(D2D1::ColorF::Red, 1)}, {1.0f, D2D1::ColorF(D2D1::ColorF::Blue, 1)} };
	ID2D1GradientStopCollection* pGradientStops{nullptr};
	float dashes[2];
};
