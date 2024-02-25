#include "ChartWindow.h"
#include "Direct2d.h"
#include <algorithm>
#include <numbers>
#include "Utils.h"

extern HINSTANCE g_hinst;
extern Direct2dWindow d2dw;

ChartWindow::ChartWindow()
{
	LoadStringW(g_hinst, IDS_CHART_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(g_hinst, IDC_CHART_NAME, szWindowClass, MAX_LOADSTRING);
}

ChartWindow::~ChartWindow()
{
    SafeRelease(&pPathAxis);
    for (uint8_t i(0); i < 3; ++i)
    {
        SafeRelease(&pPathFunc[i]);
        SafeRelease(&pPathLines[i]);
        SafeRelease(&pPathNotches[i]);
        SafeRelease(&pPathRects[i]);
        SafeRelease(&pGBrush[i]);
    }
    SafeRelease(&pSBrush);
    SafeRelease(&pGradientStops);
    SafeRelease(&pStroke);
    SafeRelease(&pRenderTarget);
    SafeRelease(&pTextFormat_1);
    SafeRelease(&pTextFormat_2);
    SafeRelease(&pDWFactory);
    SafeRelease(&pFactory);
}

/*HRESULT ChartWindow::DrawChart(std::vector<D2D1_POINT_2F>& collection, unsigned char vert_lines_pairs, unsigned char horis_lines_pairs)
{
    HRESULT hr = S_OK;
    D2D1_SIZE_F size = pRenderTarget->GetSize();
    pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Gray));
    float inc = size.height/(2*horis_lines_pairs);
    for (unsigned char i = 1; i <= horis_lines_pairs; ++i)
    {
        pRenderTarget->DrawLine({0, size.height/2 + inc*i}, {size.width, size.height/2 + inc*i}, pSBrush, 1.0f, pStroke);
        pRenderTarget->DrawLine({0, size.height/2 - inc*i}, {size.width, size.height/2 - inc*i}, pSBrush, 1.0f, pStroke);
    }
    inc = size.width/(2*vert_lines_pairs);
    for (unsigned char i = 0; i <= vert_lines_pairs; ++i)
    {
        pRenderTarget->DrawLine({size.width/2 + inc*i, 0}, {size.width/2 + inc*i, size.height}, pSBrush, 1.0f, pStroke);
        pRenderTarget->DrawLine({size.width/2 - inc*i, 0}, {size.width/2 - inc*i, size.height}, pSBrush, 1.0f, pStroke);
    }
    pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
    pRenderTarget->DrawLine({0, size.height/2}, {size.width, size.height/2}, pSBrush, 3.0f);
    pRenderTarget->DrawLine({size.width, size.height/2}, {size.width - 15, size.height/2 - 5}, pSBrush, 3.0f);
    pRenderTarget->DrawLine({size.width, size.height/2}, {size.width - 15, size.height/2 + 5}, pSBrush, 3.0f);
    pRenderTarget->DrawLine({size.width/2, 0}, {size.width/2, size.height}, pSBrush, 3.0f);
    pRenderTarget->DrawLine({size.width/2, 0}, {size.width/2 + 5, 15}, pSBrush, 3.0f);
    pRenderTarget->DrawLine({size.width/2, 0}, {size.width/2 - 5, 15}, pSBrush, 3.0f);
    if (!collection.empty())
    {
        float min_x = collection[0].x, max_x = min_x, min_y = collection[0].y, max_y = min_y;
        std::sort(collection.begin(), collection.end(), [](D2D1_POINT_2F& a, D2D1_POINT_2F& b) { return a.x < b.x; });
        for (const D2D1_POINT_2F& a : collection)
        {
            if (a.x < min_x) min_x = a.x;
            if (a.x > max_x) max_x = a.x;
            if (a.y < min_y) min_y = a.y;
            if (a.y > max_y) max_y = a.y;
        }
    }
    return hr;
}*/

HRESULT ChartWindow::DrawHistogram()
{
#define m_tip_manager m_tip_manager[d2dw.isel]
#define stored_text stored_text[d2dw.isel]
#define pPathRects pPathRects[d2dw.isel]
#define pPathLines pPathLines[d2dw.isel]
#define pPathFunc pPathFunc[d2dw.isel]
#define pPathNotches pPathNotches[d2dw.isel]
#define pGBrush pGBrush[d2dw.isel]
    HRESULT hr = S_OK;
    WCHAR text[FLOAT_TEXT_MAX]; // Текстовый буфер на любой случай
    if (d2dw.isel != 3)
    {
        if (pPathRects || need_redraw[d2dw.isel])
        {
            if (need_redraw[d2dw.isel])
            {
                // Расчёты
                unsigned int n = pValues->size();
                float min_x = (*pValues)[0], max_x = min_x;
                std::sort(pValues->begin(), pValues->end(), [](float a, float b) { return a < b; });
                float mean = 0, dispersion = 0, asymmetry = 0, excess = 0;
                for (float a : *pValues)
                {
                    mean += a;
                    if (a < min_x) min_x = a;
                    if (a > max_x) max_x = a;
                }
                mean /= n;
                d2dw.means[d2dw.isel] = mean;
                for (float a : *pValues)
                {
                    dispersion += pow(mean - a, 2);
                    asymmetry += pow(mean - a, 3);
                    excess += pow(mean - a, 4);
                }
                dispersion /= n - 1;
                d2dw.dispersions[d2dw.isel] = dispersion;
                asymmetry /= (n - 1) * pow(dispersion, 1.5);
                excess = excess / ((n - 1) * pow(dispersion, 2)) - 3;
                d2dw.Recompare();
                float k = 1 + 3.322 * log10(n);
                float interval = (max_x - min_x) / k;
                float mean_point = (mean - min_x) / (max_x + interval - min_x);

                // Установка текста в статики и создание подсказок
                _stprintf_s(text, FLOAT_TEXT_MAX, TEXT("%f"), mean);
                SetWindowText(d2dw.hSt_mean, text);
                _stprintf_s(text, FLOAT_TEXT_MAX, TEXT("%f"), dispersion);
                SetWindowText(d2dw.hSt_dispersion, text);
                _stprintf_s(text, FLOAT_TEXT_MAX, TEXT("%f"), asymmetry);
                SetWindowText(d2dw.hSt_asymmetry, text);
                _stprintf_s(text, FLOAT_TEXT_MAX, TEXT("%f"), excess);
                SetWindowText(d2dw.hSt_excess, text);
                TCHAR tip_text[256];
                _tcscpy_s(tip_text, ARRAYSIZE(tip_text), Compare(asymmetry, 0) && Compare(excess, 0) ?
                    TEXT("Распределение соответствует теоретическому нормальному распределению.") : TEXT("Распределение не соответствует теоретическому нормальному распределению."));
                RECT r1, r2, r3, r4;
                GetWindowRect(d2dw.hSt_asymmetry_text, &r1); GetWindowRect(d2dw.hSt_excess_text, &r2); GetWindowRect(d2dw.hSt_asymmetry, &r3); GetWindowRect(d2dw.hSt_excess, &r4);
                unsigned int group = d2dw.m_tip_manager.GROUP_TEORETICAL;
                d2dw.m_tip_manager.EraseGroup(group);
                d2dw.m_tip_manager.Add(group, tip_text, std::move(r1), std::move(r2), std::move(r3), std::move(r4));

                // Рисование, подсказки и прочее
                stored_text.clear();
                gradientStops[1].position = mean_point;
                SafeRelease(&pGBrush);
                SafeRelease(&pGradientStops);
                SafeRelease(&pPathRects);
                SafeRelease(&pPathLines);
                SafeRelease(&pPathFunc);
                SafeRelease(&pPathNotches);
                ID2D1GeometrySink* pSinkRects{ nullptr }, * pSinkLines{ nullptr }, * pSinkFunc{ nullptr }, * pSinkNotches{ nullptr };
                hr = pRenderTarget->CreateGradientStopCollection(gradientStops, ARRAYSIZE(gradientStops), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pGradientStops);
                if (SUCCEEDED(hr))
                    hr = pRenderTarget->CreateLinearGradientBrush(
                        D2D1::LinearGradientBrushProperties(D2D1::Point2F(offset, 0), D2D1::Point2F(size.width - offset - axis_offset, 0)),
                        pGradientStops, &pGBrush);
                if (SUCCEEDED(hr)) hr = pFactory->CreatePathGeometry(&pPathRects);
                if (SUCCEEDED(hr)) hr = pFactory->CreatePathGeometry(&pPathLines);
                if (SUCCEEDED(hr)) hr = pFactory->CreatePathGeometry(&pPathFunc);
                if (SUCCEEDED(hr)) hr = pFactory->CreatePathGeometry(&pPathNotches);
                if (SUCCEEDED(hr)) hr = pPathRects->Open(&pSinkRects);
                if (SUCCEEDED(hr)) hr = pPathLines->Open(&pSinkLines);
                if (SUCCEEDED(hr)) hr = pPathFunc->Open(&pSinkFunc);
                if (SUCCEEDED(hr)) hr = pPathNotches->Open(&pSinkNotches);
                if (SUCCEEDED(hr)) pSinkRects->SetFillMode(D2D1_FILL_MODE_WINDING); else return hr;
                float draw_point_x = offset, draw_point_y = size.height - offset;
                float dx = min_x;
                float chi = 0;
                group = d2dw.m_tip_manager.GROUP_CHART;
                d2dw.m_tip_manager.EraseGroup(group);
                for (unsigned int i = 1; i <= ceil(k); ++i)
                {
                    // Гистограмма, график и линии
                    dx += interval;
                    unsigned int freq = 0;
                    auto iter = std::lower_bound(pValues->begin(), pValues->end(), dx - interval);
                    if (iter == pValues->end()) [[unlikely]] break;
                    for (; iter != pValues->end() && *iter < dx; ++iter)
                        ++freq;
                    float x1 = (dx - interval - min_x) / (interval * ceil(k)) * (size.width - 2 * offset - axis_offset) + offset;
                    float x2 = (dx - min_x) / (interval * ceil(k)) * (size.width - 2 * offset - axis_offset) + offset;
                    float y1 = (size.height - 2 * offset - axis_offset) - (size.height - 2 * offset - axis_offset) * freq / n + offset + axis_offset;
                    float y2 = size.height - offset;
                    if (freq != 0)
                    {
                        pSinkRects->BeginFigure(D2D1::Point2F(x1, y2), D2D1_FIGURE_BEGIN_FILLED);
                        D2D1_POINT_2F points[] = { {x1, y1}, {x2, y1}, {x2, y2} };
                        pSinkRects->AddLines(points, ARRAYSIZE(points));
                        pSinkRects->EndFigure(D2D1_FIGURE_END_OPEN);
                    }
                    pSinkFunc->BeginFigure(D2D1::Point2F(draw_point_x, draw_point_y), D2D1_FIGURE_BEGIN_HOLLOW);
                    pSinkFunc->AddLine(D2D1::Point2F(x2, draw_point_y - (size.height - y1 - offset)));
                    pSinkFunc->EndFigure(D2D1_FIGURE_END_OPEN);
                    draw_point_x = x2;
                    draw_point_y -= size.height - y1 - offset;
                    pSinkLines->BeginFigure(D2D1::Point2F(x2, size.height - offset), D2D1_FIGURE_BEGIN_HOLLOW);
                    D2D1_POINT_2F points[] = { {x2, size.height - offset}, {x2, draw_point_y}, {offset, draw_point_y} };
                    pSinkLines->AddLines(points, ARRAYSIZE(points));
                    pSinkLines->EndFigure(D2D1_FIGURE_END_OPEN);

                    // Эмпирические и теоретические частоты
                    float empirical = freq / static_cast<float>(n), symb_temp = symbols(empirical);
                    stored_text.push_back({ D2D1::RectF(x1, y1 - 30.0f, x2, y1 - 15.0f), D2D1::ColorF(D2D1::ColorF::DarkBlue),
                        {DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_FAR}, empirical });
                    float teor_freq = Simpson(dx - interval, dx,
                        [mean, deviation = pow(dispersion, 0.5)](float x) { return pow(std::numbers::e, -0.5 * pow((x - mean) / deviation, 2)) / (deviation * pow(2 * std::numbers::pi, 0.5)); });
                    stored_text.push_back({ D2D1::RectF(x1, y1 - 15.0f, x2, y1), D2D1::ColorF(D2D1::ColorF::DarkRed),
                        {DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_FAR}, teor_freq });
                    chi += pow(empirical - teor_freq, 2) / teor_freq;

                    // Подсказки на частотах
                    RECT tip_rect(x1, y1 - 30.0f, x2, y2);
                    MapWindowPoints(m_hwnd, NULL, reinterpret_cast<LPPOINT>(&tip_rect), 2);
                    _stprintf_s(tip_text, ARRAYSIZE(tip_text),
                        TEXT("Диапазон %f - %f:\nЭмпирическая частота: %f;\nТеоретическая частота: %f;\nЧисло попаданий: %u."), dx - interval, dx, empirical, teor_freq, freq);
                    d2dw.m_tip_manager.Add(group, tip_text, std::move(tip_rect));

                    // Разметка и числа на оси X
                    if (i >= 2)
                    {
                        stored_text.push_back({ D2D1::RectF(x1 - axis_offset, size.height - offset + notch, x1 + axis_offset, size.height),
                            D2D1::ColorF(D2D1::ColorF::Black), {DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_NEAR}, dx - interval });
                        pSinkNotches->BeginFigure(D2D1::Point2F(x1, size.height - offset - notch), D2D1_FIGURE_BEGIN_HOLLOW);
                        pSinkNotches->AddLine(D2D1::Point2F(x1, size.height - offset + notch));
                        pSinkNotches->EndFigure(D2D1_FIGURE_END_OPEN);
                    }
                    else
                        stored_text.push_back({ D2D1::RectF(offset - axis_offset, size.height - offset + notch, offset + axis_offset, size.height),
                            D2D1::ColorF(D2D1::ColorF::Black), {DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_NEAR}, min_x });
                    if (i == ceil(k)) [[unlikely]] // Последний интервал
                    {
                        stored_text.push_back({ D2D1::RectF((size.width - 2 * offset) / 8, 0, (size.width - 2 * offset) / 4, 2 * offset / 3),
                            D2D1::ColorF(D2D1::ColorF::DarkBlue), {DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_FAR}, std::get<float>(stored_text[0]) });
                        stored_text.push_back({ D2D1::RectF((size.width - 2 * offset) / 8, offset, (size.width - 2 * offset) / 4, 2 * offset / 3 + axis_offset),
                            D2D1::ColorF(D2D1::ColorF::DarkRed), {DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_FAR}, std::get<float>(stored_text[1]) });
                        stored_text.push_back({D2D1::RectF(x2 - axis_offset, size.height - offset + notch, x2 + axis_offset, size.height),
                            D2D1::ColorF(D2D1::ColorF::Black), {DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_NEAR}, dx});
                        pSinkNotches->BeginFigure(D2D1::Point2F(x2, size.height - offset - notch), D2D1_FIGURE_BEGIN_HOLLOW);
                        pSinkNotches->AddLine(D2D1::Point2F(x2, size.height - offset + notch));
                        pSinkNotches->EndFigure(D2D1_FIGURE_END_OPEN);

                        // Здесь удобно досчитать и установить подсказку про критерий согласия
                        chi *= i;
                        float X2 = 1 - Simpson(0, chi,[r = static_cast<float>(i) - 1](float x){ return pow(0.5, r / 2) * pow(x, r / 2 - 1) * pow(std::numbers::e, -x / 2) / std::tgamma(r / 2); });
                        _stprintf_s(text, FLOAT_TEXT_MAX, TEXT("%f"), X2);
                        SetWindowText(d2dw.hSt_pearson_result, text);
                        d2dw.pearson_p[d2dw.isel] = X2;
                        d2dw.TrySetPearsonTip();
                    }
                }
                hr = pSinkRects->Close();
                if (SUCCEEDED(hr)) hr = pSinkLines->Close();
                if (SUCCEEDED(hr)) hr = pSinkFunc->Close();
                if (SUCCEEDED(hr)) hr = pSinkNotches->Close();
                SafeRelease(&pSinkRects);
                SafeRelease(&pSinkLines);
                SafeRelease(&pSinkFunc);
                SafeRelease(&pSinkNotches);
                if (FAILED(hr)) [[unlikely]] return hr;
            }
            pRenderTarget->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), NULL, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, D2D1::IdentityMatrix(), d2dw.actual[d2dw.isel] ? 1.0f : 0.5f), NULL);
            pRenderTarget->FillGeometry(pPathRects, pGBrush);
            pRenderTarget->DrawGeometry(pPathRects, pSBrush);
            pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Green, 1));
            pRenderTarget->DrawGeometry(pPathLines, pSBrush, 0.5f, pStroke);
            pRenderTarget->DrawGeometry(pPathFunc, pSBrush, 4.0f);
            pRenderTarget->DrawLine({2*(size.width-2*offset)/3-50, offset}, {2*(size.width-2*offset)/3, offset}, pSBrush, 4.0f);
            pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
            pRenderTarget->DrawGeometry(pPathNotches, pSBrush, 3.0f);
            for (auto& p : stored_text)
            {
                hr = pTextFormat_2->SetTextAlignment(std::get<2>(p).first);
                if (SUCCEEDED(hr)) hr = pTextFormat_2->SetParagraphAlignment(std::get<2>(p).second);
                if (FAILED(hr)) [[unlikely]] return hr;
                pSBrush->SetColor(std::get<D2D1_COLOR_F>(p));
                float flt = std::get<float>(p);
                _stprintf_s(text, FLOAT_TEXT_MAX, TEXT("%f"), flt);
                pRenderTarget->DrawText(text, symbols(flt), pTextFormat_2, std::get<D2D1_RECT_F>(p), pSBrush);
            }
            pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
            hr = pTextFormat_2->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            if (SUCCEEDED(hr)) hr = pTextFormat_2->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
            if (FAILED(hr)) [[unlikely]] return hr;
            for (auto& p : legenda[0])
                pRenderTarget->DrawText(p.second.c_str(), p.second.size(), pTextFormat_2, p.first, pSBrush);
            pRenderTarget->PopLayer();
        }
    }
    else
    {
#undef pPathFunc
#undef pPathLines
#undef pGBrush
        uint8_t count = 0; if (d2dw.actual[0]) ++count; if (d2dw.actual[1]) ++count; if (d2dw.actual[2]) ++count;
        hr = pTextFormat_2->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        if (SUCCEEDED(hr)) hr = pTextFormat_2->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
        if (FAILED(hr)) [[unlikely]] return hr;
        if (count >= 2)
        {
            pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
            for (uint8_t i = 0, pos = 0; i < ARRAYSIZE(d2dw.actual); ++i)
            {
                if (d2dw.actual[i])
                {
                    if (pos != 2)
                    {
                        pRenderTarget->DrawLine({!pos ? (size.width-2*offset)/4-50 : 3*(size.width-2*offset)/4-50, 2*offset/3+axis_offset-7}, {!pos ? (size.width-2*offset)/4 : 3*(size.width-2*offset)/4, 2*offset/3+axis_offset-7}, pSBrush, 4.0f);
                        switch (i)
                        {
                            case 0:
                            {
                                pRenderTarget->DrawGeometry(pPathFunc[0], pSBrush, 4.0f);
                                pRenderTarget->DrawGeometry(pPathLines[0], pSBrush, 0.5f, pStroke);
                                TCHAR t[] = L" — Функция распределения X";
                                pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
                                pRenderTarget->DrawText(t, ARRAYSIZE(t), pTextFormat_2,
                                    D2D1::RectF((size.width-2*offset)/4, 2*offset/3, 2*(size.width-2*offset)/3, 2*offset/3+axis_offset), pSBrush);
                                pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Green));
                                break;
                            }
                            case 1:
                            {
                                pRenderTarget->DrawGeometry(pPathFunc[1], pSBrush, 4.0f);
                                pRenderTarget->DrawGeometry(pPathLines[1], pSBrush, 0.5f, pStroke);
                                TCHAR t[] = L" — Функция распределения Y";
                                pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
                                pRenderTarget->DrawText(t, ARRAYSIZE(t), pTextFormat_2,
                                    D2D1::RectF(!pos ? (size.width-2*offset)/4 : 3*(size.width-2*offset)/4, 2*offset/3, !pos ? 3*(size.width-2*offset)/4 : size.width-offset, 2*offset/3+axis_offset), pSBrush);
                                if (!pos) pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Green));
                                break;
                            }
                            case 2:
                            {
                                pRenderTarget->DrawGeometry(pPathFunc[2], pSBrush, 4.0f);
                                pRenderTarget->DrawGeometry(pPathLines[2], pSBrush, 0.5f, pStroke);
                                TCHAR t[] = L" — Функция распределения Z";
                                pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
                                pRenderTarget->DrawText(t, ARRAYSIZE(t), pTextFormat_2,
                                    D2D1::RectF(!pos ? (size.width-2*offset)/4 : 3*(size.width-2*offset)/4, 2*offset/3, !pos ? 3*(size.width-2*offset)/4 : size.width-offset, 2*offset/3+axis_offset), pSBrush);
                                break;
                            }
                        }
                        ++pos;
                    }
                    else
                    {
                        pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue));
                        pRenderTarget->DrawLine({(size.width-2*offset)/2-50, 2*offset/3}, {(size.width-2*offset)/2, 2*offset/3}, pSBrush, 4.0f);
                        pRenderTarget->DrawGeometry(pPathFunc[2], pSBrush, 4.0f);
                        pRenderTarget->DrawGeometry(pPathLines[2], pSBrush, 0.5f, pStroke);
                        TCHAR t[] = L" — Функция распределения Z";
                        pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
                        pRenderTarget->DrawText(t, ARRAYSIZE(t), pTextFormat_2,
                            D2D1::RectF((size.width-2*offset)/2, 2*offset/3+7, size.width-offset, 2*offset/3+7), pSBrush);
                    }
                }

            }
        }
        else if (count == 1)
        {
            pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
            pRenderTarget->DrawLine({(size.width-2*offset)/2-50, 2*offset/3+axis_offset-7}, {(size.width-2*offset)/2, 2*offset/3+axis_offset-7}, pSBrush, 4.0f);
            for (uint8_t i = 0, pos = 0; i<ARRAYSIZE(d2dw.actual); ++i)
            {
                if (d2dw.actual[i])
                {
                    switch (i)
                    {
                        case 0:
                        {
                            pRenderTarget->DrawGeometry(pPathFunc[0], pSBrush, 4.0f);
                            pRenderTarget->DrawGeometry(pPathLines[0], pSBrush, 0.5f, pStroke);
                            TCHAR t[] = L" — Функция распределения X";
                            pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
                            pRenderTarget->DrawText(t, ARRAYSIZE(t), pTextFormat_2,
                                D2D1::RectF((size.width-2*offset)/2, 2*offset/3, size.width-offset, 2*offset/3+axis_offset), pSBrush);
                            break;
                        }
                        case 1:
                        {
                            pRenderTarget->DrawGeometry(pPathFunc[1], pSBrush, 4.0f);
                            pRenderTarget->DrawGeometry(pPathLines[1], pSBrush, 0.5f, pStroke);
                            TCHAR t[] = L" — Функция распределения Y";
                            pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
                            pRenderTarget->DrawText(t, ARRAYSIZE(t), pTextFormat_2,
                                D2D1::RectF((size.width-2*offset)/2, 2*offset/3, size.width-offset, 2*offset/3+axis_offset), pSBrush);
                            break;
                        }
                        case 2:
                        {
                            pRenderTarget->DrawGeometry(pPathFunc[2], pSBrush, 4.0f);
                            pRenderTarget->DrawGeometry(pPathLines[2], pSBrush, 0.5f, pStroke);
                            TCHAR t[] = L" — Функция распределения Z";
                            pSBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
                            pRenderTarget->DrawText(t, ARRAYSIZE(t), pTextFormat_2,
                                D2D1::RectF((size.width-2*offset)/2, 2*offset/3, size.width-offset, 2*offset/3+axis_offset), pSBrush);
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    pRenderTarget->DrawGeometry(pPathAxis, pSBrush, 3.0f);
    hr = pTextFormat_1->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    if (SUCCEEDED(hr)) hr = pTextFormat_1->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
    if (SUCCEEDED(hr)) pRenderTarget->DrawText(L"P", 1, pTextFormat_1, D2D1::RectF(offset - axis_offset, 0, offset + axis_offset, offset), pSBrush);
    else return hr;
    hr = pTextFormat_1->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    if (SUCCEEDED(hr)) hr = pTextFormat_1->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    if (SUCCEEDED(hr)) pRenderTarget->DrawText(L"X", 1, pTextFormat_1, D2D1::RectF(size.width - offset, size.height - offset - axis_offset, size.width, size.height - offset + axis_offset), pSBrush);
    else return hr;
    hr = pTextFormat_2->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    if (SUCCEEDED(hr)) hr = pTextFormat_2->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
    if (SUCCEEDED(hr))
        for (auto& p : common_text)
        {
            _stprintf_s(text, FLOAT_TEXT_MAX, TEXT("%f"), p.second);
            pRenderTarget->DrawText(text, (p.second < 0.99 ? 3 : 1), pTextFormat_2, p.first, pSBrush);
        }
    else return hr;
    pRenderTarget->DrawText(L"0", 1, pTextFormat_2, D2D1::RectF(0.0f, size.height - offset - axis_offset, common_text[0].first.right, size.height - offset), pSBrush);
    return hr;
#undef m_tip_manager
#undef stored_text
#undef pPathRects
#undef pPathNotches
}

HRESULT ChartWindow::CreatePathTemplate()
{
    HRESULT hr = S_OK;
    pFactory->CreatePathGeometry(&pPathAxis);
    ID2D1GeometrySink* pSink = nullptr;
    pPathAxis->Open(&pSink);
    {
        pSink->BeginFigure(D2D1::Point2F(offset, offset), D2D1_FIGURE_BEGIN_HOLLOW);
        pSink->AddLine(D2D1::Point2F(offset, size.height - offset + notch));
        pSink->EndFigure(D2D1_FIGURE_END_OPEN);
        pSink->BeginFigure(D2D1::Point2F(offset - notch, size.height - offset), D2D1_FIGURE_BEGIN_HOLLOW);
        pSink->AddLine(D2D1::Point2F(size.width - offset, size.height - offset));
        pSink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
    {
        pSink->BeginFigure(D2D1::Point2F(size.width - offset - 15.0f, size.height - offset - 5), D2D1_FIGURE_BEGIN_HOLLOW);
        D2D1_POINT_2F points[] = { {size.width - offset - 15.0f, size.height - offset - 5}, {size.width - offset, size.height - offset}, {size.width - offset - 15.0f, size.height - offset + 5} };
        pSink->AddLines(points, ARRAYSIZE(points));
        pSink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
    {
        pSink->BeginFigure(D2D1::Point2F(offset - 5, offset + 15.0f), D2D1_FIGURE_BEGIN_HOLLOW);
        D2D1_POINT_2F points[] = { {offset - 5, offset + 15.0f}, {offset, offset}, {offset + 5, offset + 15.0f} };
        pSink->AddLines(points, ARRAYSIZE(points));
        pSink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
    for (float kf = 1; kf > 0.01; kf -= 0.1)
    {
        float y = (1 - kf) * (size.height - 2 * offset - axis_offset) + offset + axis_offset;
        pSink->BeginFigure(D2D1::Point2F(offset - notch, y), D2D1_FIGURE_BEGIN_HOLLOW);
        pSink->AddLine({ offset + notch, y });
        pSink->EndFigure(D2D1_FIGURE_END_OPEN);
        common_text.push_back({D2D1::RectF(0.0f, y - axis_offset, offset-5.0f, y), kf});
    }
    pSink->Close();
    SafeRelease(&pSink);
    legenda[0].emplace_back(D2D1::RectF((size.width - 2*offset)/4, 0, 2*(size.width - 2*offset)/3, 2*offset/3), L" — эмпирическая частота");
    legenda[0].emplace_back(D2D1::RectF((size.width - 2*offset)/4, offset, 2*(size.width - 2*offset)/3, 2*offset/3 + axis_offset), L" — теоретическая частота");
    legenda[0].emplace_back(D2D1::RectF(2*(size.width - 2*offset)/3, offset - 7, size.width - offset, offset + 7), L" — график функции распределения");
    return hr;
}

LRESULT ChartWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            scale = GetDpiForWindow(m_hwnd) / 96.0f;
            HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);

            if (SUCCEEDED(hr))
            {
                RECT rc;
                GetClientRect(m_hwnd, &rc);
                D2D1_SIZE_U currentSize = D2D1::SizeU(rc.right, rc.bottom);
                hr = pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hwnd, currentSize), &pRenderTarget);
                if (SUCCEEDED(hr))
                {
                    size = pRenderTarget->GetSize();
                    hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pSBrush);
                }
                if (SUCCEEDED(hr))
                {
                    dashes[0] = 5.0f;
                    dashes[1] = 5.0f;
                    hr = pFactory->CreateStrokeStyle(
                        D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE_SQUARE, D2D1_LINE_JOIN_MITER, 10.0f, D2D1_DASH_STYLE_CUSTOM, 0.0f),
                        dashes, ARRAYSIZE(dashes), &pStroke);
                }
                if (SUCCEEDED(hr))
                    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pDWFactory), reinterpret_cast<IUnknown**>(&pDWFactory));
                if (SUCCEEDED(hr))
                {
                    hr = pDWFactory->CreateTextFormat(
                        L"Times New Roman", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 26.0f, L"ru-RU", &pTextFormat_1);
                }
                if (SUCCEEDED(hr))
                {
                    hr = pDWFactory->CreateTextFormat(
                        L"Times New Roman", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"ru-RU", &pTextFormat_2);
                }
                if (SUCCEEDED(hr))
                    hr = CreatePathTemplate();
            }

            if (FAILED(hr))
            {
                MessageBox(NULL, TEXT("Не удалось инициализировать графические ресурсы."), TEXT("Ошибка"), MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
                return -1;
            }

            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);

            HRESULT hr;
            pRenderTarget->BeginDraw();
            pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
            hr = DrawHistogram();
            if (SUCCEEDED(hr)) hr = pRenderTarget->EndDraw();
            if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) [[unlikely]]
            {
                MessageBox(NULL, TEXT("Произошла ошибка во время отрисовки гистограммы."), TEXT("Ошибка"), MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
                PostQuitMessage(0);
            }

            EndPaint(m_hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
    return 0;
}