#include "Direct2d.h"
#include <fstream>
#include "Utils.h"
#include <boost/math/distributions/fisher_f.hpp>

extern int g_cmd;
extern HINSTANCE g_hinst;
extern Direct2dWindow d2dw;

#define pearson_a pearson_a[isel]
#define pearson_p pearson_p[isel]
#define values values[isel]
#define strings strings[isel]

Direct2dWindow::Direct2dWindow()
{
    LoadStringW(g_hinst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(g_hinst, IDC_WINAPI, szWindowClass, MAX_LOADSTRING);
    mouse_hook = SetWindowsHookEx(WH_MOUSE, &Direct2dWindow::MouseHook, NULL, GetCurrentThreadId());
    keyboard_hook = SetWindowsHookEx(WH_KEYBOARD, &Direct2dWindow::KeyboardHook, NULL, GetCurrentThreadId());
}

Direct2dWindow::~Direct2dWindow()
{
    UnhookWindowsHookEx(mouse_hook);
    UnhookWindowsHookEx(keyboard_hook);
}

bool Direct2dWindow::Verify(HWND edit)
{
    TCHAR buf[FLOAT_TEXT_MAX];
    Edit_GetText(edit, buf, FLOAT_TEXT_MAX);
    if (_tcscmp(buf, TEXT("")) == 0)
    {
        MessageBox(NULL, TEXT("Необходимо ввести значение в поле редактирования справа."), TEXT("Недопустимый ввод"), MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
        return false;
    }
    _set_errno(0);
    TCHAR* ptr;
    float valid = _tcstof(buf, &ptr);
    if (valid == 0 && _tcscmp(buf, TEXT("0")) != 0 || _tcscmp(ptr, TEXT("\0")) != 0)
    {
        MessageBox(NULL, TEXT("Введено некорректное значение."), TEXT("Недопустимый ввод"), MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
        return false;
    }
    if (errno == ERANGE)
    {
        MessageBox(NULL, TEXT("Введённое значение вызвало переполнение."), TEXT("Недопустимый ввод"), MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
        _set_errno(0);
        return false;
    }
    return true;
}

bool VerifyForTip(std::optional<float>& lev, const HWND& hedit)
{
    TCHAR buf[FLOAT_TEXT_MAX], * ptr;
    auto SetBack = [&]()
    {
        if (lev.has_value())
        {
            _stprintf_s(buf, FLOAT_TEXT_MAX, TEXT("%f"), lev.value());
            SetWindowText(hedit, buf);
        }
    };
    Edit_GetText(hedit, buf, FLOAT_TEXT_MAX);
    if (_tcscmp(buf, TEXT("")) == 0)
    {
        SetBack();
        return false;
    }
    float valid = _tcstof(buf, &ptr);
    if (valid == 0 && _tcscmp(buf, TEXT("0")) != 0 || _tcscmp(ptr, TEXT("\0")) != 0)
    {
        MessageBox(NULL, TEXT("В поле \"Уровень значимости\" введено некорректное значение."), TEXT("Недопустимый ввод"), MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONWARNING);
        SetBack();
        return false;
    }
    if (lev != valid)
    {
        if (valid < 0 || valid > 1)
        {
            MessageBox(NULL, TEXT("Критерий значимости должен находится в интервале [0; 1]."), TEXT("Недопустимый ввод"), MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONWARNING);
            SetBack();
            return false;
        }
        lev = valid;
    }
    return true;
}

Direct2dWindow::MatrixWindow::MatrixWindow()
{
    _tcscpy_s(szTitle, MAX_LOADSTRING, L"MATRIX_TITLE");
    _tcscpy_s(szWindowClass, MAX_LOADSTRING, L"MATRIX_CLASS");
}

LRESULT Direct2dWindow::MatrixWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            RECT r;
            GetClientRect(m_hwnd, &r);
            for (uint8_t i(0), index(0); i < 3; ++i)
                for (uint8_t j(0); j < 3; ++j)
                {
                    statics[i][j] = CreateWindow(WC_STATIC, NULL, SS_CENTER | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                        j * r.right/3, i * r.bottom / 3 + r.bottom / 6 - 7, r.right / 3, r.bottom / 3, m_hwnd, 0, g_hinst, NULL);
                    if (j > i)
                        SetWindowText(statics[i][j], TEXT("-"));
                    else if (j < i)
                        SetWindowText(statics[i][j], TEXT("0"));
                    else
                        SetWindowText(statics[i][j], TEXT("1"));
                }
            break;
        }
        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            //SetBkMode(hdc, TRANSPARENT);
            break;
        }
        default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT Direct2dWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Размеры и окно гистограммы
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int height = rc.bottom - 20;
        int width = height;
        if (width > rc.right - 20)
        {
            width = rc.right - 20;
            height = width;
        }
        chartWindow.Create(g_hinst, TEXT("Chart"), WS_CHILD | WS_BORDER, NULL, rc.right - width - 10, 10, width, height, m_hwnd);
        ShowWindow(chartWindow.Window(), g_cmd);

        // Начало размещения элементов, вкладки
        LONG right_x = 295, left_x = 5;
        RECT coords(left_x, 10, right_x, height);
        hwndTab = CreateWindow(WC_TABCONTROL, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_DLGFRAME | TCS_MULTILINE, coords.left, coords.top, coords.right, coords.bottom, m_hwnd, NULL, g_hinst, NULL);
        TCITEM tab_item;
        tab_item.mask = TCIF_TEXT | TCIF_IMAGE;
        tab_item.iImage = -1;
        tab_item.pszText = const_cast<LPWSTR>(TEXT("Величина X"));
        TabCtrl_InsertItem(hwndTab, 0, &tab_item);
        tab_item.pszText = const_cast<LPWSTR>(TEXT("Величина Y"));
        TabCtrl_InsertItem(hwndTab, 1, &tab_item);
        tab_item.pszText = const_cast<LPWSTR>(TEXT("Величина Z"));
        TabCtrl_InsertItem(hwndTab, 2, &tab_item);
        tab_item.pszText = const_cast<LPWSTR>(TEXT("Сравнение величин"));
        TabCtrl_InsertItem(hwndTab, 3, &tab_item);
        TabCtrl_AdjustRect(hwndTab, FALSE, &coords);
        isel = TabCtrl_GetCurSel(hwndTab);

        // Кнопки и статики до таблицы
        hBt_add = CreateWindow(WC_BUTTON, TEXT("Добавить величину:"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, coords.left += 5, coords.top += 10, coords.right = 175, 20, m_hwnd, (HMENU)BT_ADD, g_hinst, NULL);
        hEd_add = CreateWindow(WC_EDIT, NULL, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, coords.right, coords.top, right_x - (coords.right = coords.left + coords.right + 10) - 5, 20, m_hwnd, (HMENU)ED_ADD, g_hinst, NULL);
        hBt_gen = CreateWindow(WC_BUTTON, TEXT("Сгенерировать"), WS_CHILD | WS_VISIBLE, coords.left, coords.top += 40, 110, 20, m_hwnd, (HMENU)BT_GEN, g_hinst, NULL);
        hEd_gen = CreateWindow(WC_EDIT, NULL, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, coords.left + 120, coords.top, right_x - coords.right - 5, 20, m_hwnd, (HMENU)ED_GEN, g_hinst, NULL);
        hSt_text_gen1 = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD | WS_VISIBLE, coords.left + 130 + right_x - coords.right - 5, coords.top, right_x - (coords.left + 130 + right_x - coords.right - 5) - 5, 20, m_hwnd, (HMENU)ST_TEXT_GEN_1, g_hinst, NULL);
        SetWindowText(hSt_text_gen1, TEXT("опытов"));
        hSt_text_gen2 = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD | WS_VISIBLE, coords.left, coords.top += 30, coords.right - coords.left - 10, 20, m_hwnd, (HMENU)ST_TEXT_GEN_2, g_hinst, NULL);
        SetWindowText(hSt_text_gen2, TEXT("в диапазоне от"));
        hEd_gen_from = CreateWindow(WC_EDIT, NULL, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, coords.right, coords.top, right_x - coords.right - 5, 20, m_hwnd, (HMENU)ED_GEN_FROM, g_hinst, NULL);
        hSt_text_gen3 = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD | WS_VISIBLE, coords.left, coords.top += 30, coords.right - coords.left - 10, 20, m_hwnd, (HMENU)ST_TEXT_GEN_3, g_hinst, NULL);
        SetWindowText(hSt_text_gen3, TEXT("до"));
        hEd_gen_to = CreateWindow(WC_EDIT, NULL, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, coords.right, coords.top, right_x - coords.right - 5, 20, m_hwnd, (HMENU)ED_GEN_TO, g_hinst, NULL);
        hStatic_pearson = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD | WS_VISIBLE, coords.left, coords.top += 40, coords.right - coords.left - 10, 20, m_hwnd, (HMENU)ST_PEARSON, g_hinst, NULL);
        SetWindowText(hStatic_pearson, TEXT("Уровень значимости:"));
        hEdit_pearson = CreateWindow(WC_EDIT, NULL, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, coords.right, coords.top, right_x - coords.right - 5, 20, m_hwnd, (HMENU)ED_PEARSON, g_hinst, NULL);
        Edit_LimitText(hEd_gen, 10);

        // Кнопки и статики после таблицы
        hSt_pearson_result_text = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD | WS_VISIBLE, coords.left += 10, coords.bottom -= 30, coords.right = 150, 30, m_hwnd, (HMENU)ST_PEARSON_RESULT_TEXT, g_hinst, NULL);
        SetWindowText(hSt_pearson_result_text, TEXT("Расчётный уровень\nзначимости:"));
        hSt_pearson_result = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD | WS_VISIBLE, coords.left + coords.right, coords.bottom + 8, right_x - coords.right - coords.left - 15, 20, m_hwnd, (HMENU)ST_PEARSON_RESULT, g_hinst, NULL);
        hSt_excess_text = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD | WS_VISIBLE, coords.left, coords.bottom -= 30, coords.right, 20, m_hwnd, (HMENU)ST_EXCESS_TEXT, g_hinst, NULL);
        SetWindowText(hSt_excess_text, TEXT("Эксцесс:"));
        hSt_excess = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD | WS_VISIBLE, coords.left + coords.right, coords.bottom, right_x - coords.right - coords.left - 15, 20, m_hwnd, (HMENU)ST_EXCESS, g_hinst, NULL);
        hSt_asymmetry_text = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD | WS_VISIBLE, coords.left, coords.bottom -= 30, coords.right, 20, m_hwnd, (HMENU)ST_ASYMMETRY_TEXT, g_hinst, NULL);
        SetWindowText(hSt_asymmetry_text, TEXT("Асимметрия:"));
        hSt_asymmetry = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD | WS_VISIBLE, coords.left + coords.right, coords.bottom, right_x - coords.right - coords.left - 15, 20, m_hwnd, (HMENU)ST_ASYMMETRY, g_hinst, NULL);
        hSt_dispersion_text = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD | WS_VISIBLE, coords.left, coords.bottom -= 30, coords.right, 20, m_hwnd, (HMENU)ST_DISPERSION_TEXT, g_hinst, NULL);
        SetWindowText(hSt_dispersion_text, TEXT("Дисперсия:"));
        hSt_dispersion = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD | WS_VISIBLE, coords.left + coords.right, coords.bottom, right_x - coords.right - coords.left - 15, 20, m_hwnd, (HMENU)ST_DISPERSION, g_hinst, NULL);
        hSt_mean_text = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD | WS_VISIBLE, coords.left, coords.bottom -= 30, coords.right, 20, m_hwnd, (HMENU)ST_MEAN_TEXT, g_hinst, NULL);
        SetWindowText(hSt_mean_text, TEXT("Мат. ожидание:"));
        hSt_mean = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD | WS_VISIBLE, coords.left + coords.right, coords.bottom, right_x - coords.right - coords.left - 15, 20, m_hwnd, (HMENU)ST_MEAN, g_hinst, NULL);
        SetWindowText(hSt_mean, TEXT("-"));
        SetWindowText(hSt_dispersion, TEXT("-"));
        SetWindowText(hSt_asymmetry, TEXT("-"));
        SetWindowText(hSt_excess, TEXT("-"));
        SetWindowText(hSt_pearson_result, TEXT("-"));

        // Кнопки под таблицей
        hBt_go = CreateWindow(WC_BUTTON, L"Выполнить расчёт", WS_CHILD | WS_VISIBLE | WS_DISABLED, coords.left, coords.bottom -= 50, right_x - (coords.left -= 10) - 5, 30, m_hwnd, (HMENU)BT_GO, g_hinst, NULL);
        hBt_load = CreateWindow(WC_BUTTON, L"Загрузить", WS_CHILD | WS_VISIBLE, coords.left, coords.bottom -= 30, coords.right = (right_x - coords.left - 5)/3, 20, m_hwnd, (HMENU)BT_LOAD, g_hinst, NULL);
        hBt_save = CreateWindow(WC_BUTTON, L"Сохранить", WS_CHILD | WS_VISIBLE | WS_DISABLED, coords.left + coords.right, coords.bottom, coords.right, 20, m_hwnd, (HMENU)BT_SAVE, g_hinst, NULL);
        hBt_clear = CreateWindow(WC_BUTTON, L"Очистить", WS_CHILD | WS_VISIBLE | WS_DISABLED, coords.left + coords.right*2, coords.bottom, coords.right, 20, m_hwnd, (HMENU)BT_CLEAR, g_hinst, NULL);
        
        // Таблица (List View)
        hListView = CreateWindow(WC_LISTVIEW, NULL, LVS_REPORT | LVS_NOSORTHEADER | WS_CHILD | WS_VISIBLE | WS_BORDER, coords.left, coords.top, right_x - coords.left - 5, coords.bottom - (coords.top += 40), m_hwnd, (HMENU)LW1, g_hinst, NULL);
        ListView_SetBkColor(hListView, 0xCCCCCC);
        LVCOLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_LEFT;
        lvc.pszText = (LPWSTR)TEXT("Номер опыта");
        lvc.cx = 100;
        lvc.iSubItem = 0;
        ListView_InsertColumn(hListView, 0, &lvc);
        lvc.pszText = (LPWSTR)TEXT("Значение величины");
        lvc.cx = 150;
        lvc.iSubItem = 1;
        ListView_InsertColumn(hListView, 1, &lvc);
        ListView_SetExtendedListViewStyle(hListView, LVS_EX_BORDERSELECT);
        ListView_SetExtendedListViewStyle(hListView, LVS_EX_GRIDLINES);
        ListView_SetExtendedListViewStyle(hListView, LVS_EX_AUTOSIZECOLUMNS);

        // Вкладка сравнения
        SetRect(&coords, left_x, 10, right_x, height);
        TabCtrl_AdjustRect(hwndTab, FALSE, &coords);
        hSt_xy_text_comp = CreateWindow(WC_STATIC, NULL, SS_CENTER | WS_CHILD, coords.left, coords.top += 10, right_x - (coords.left += 10) - 10, 20, m_hwnd, (HMENU)ST_XY_TEXT_COMP, g_hinst, NULL);
        SetWindowText(hSt_xy_text_comp, TEXT("Величины X и Y:"));
        hSt_xy_text_level = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD, coords.left, coords.top += 30, coords.right = 150, 20, m_hwnd, (HMENU)ST_XY_TEXT_LEVEL, g_hinst, NULL);
        SetWindowText(hSt_xy_text_level, TEXT("Уровень значимости:"));
        hEdit_xy = CreateWindow(WC_EDIT, NULL, WS_BORDER | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL, coords.right, coords.top, right_x - (coords.right = coords.left + coords.right + 10) - 10, 20, m_hwnd, (HMENU)ED_XY, g_hinst, NULL);
        hSt_xy_text_fisher = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD, coords.left, coords.top += 30, coords.right - coords.left - 10, 30, m_hwnd, (HMENU)ST_XY_TEXT_FISHER, g_hinst, NULL);
        SetWindowText(hSt_xy_text_fisher, TEXT("Критерий Фишера:"));
        hSt_xy_fisher = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD, coords.right, coords.top, right_x - coords.right - 10, 20, m_hwnd, (HMENU)ST_XY_FISHER, g_hinst, NULL);
        hSt_xy_text_cor = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD, coords.left, coords.top += 30, coords.right - coords.left - 10, 30, m_hwnd, (HMENU)ST_XY_TEXT_COR, g_hinst, NULL);
        SetWindowText(hSt_xy_text_cor, TEXT("Коэф. парной\nкорреляции:"));
        hSt_xy_cor = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD, coords.right, coords.top + 7, right_x - coords.right - 10, 20, m_hwnd, (HMENU)ST_XY_COR, g_hinst, NULL);
        SetWindowText(hSt_xy_cor, TEXT("-"));
        SetWindowText(hSt_xy_fisher, TEXT("-"));

        hSt_xz_text_comp = CreateWindow(WC_STATIC, NULL, SS_CENTER | WS_CHILD, coords.left, coords.top += 50, right_x - coords.left - 10, 20, m_hwnd, (HMENU)ST_XZ_TEXT_COMP, g_hinst, NULL);
        SetWindowText(hSt_xz_text_comp, TEXT("Величины X и Z:"));
        hSt_xz_text_level = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD, coords.left, coords.top += 30, coords.right = 150, 20, m_hwnd, (HMENU)ST_XZ_TEXT_LEVEL, g_hinst, NULL);
        SetWindowText(hSt_xz_text_level, TEXT("Уровень значимости:"));
        hEdit_xz = CreateWindow(WC_EDIT, NULL, WS_BORDER | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL, coords.right, coords.top, right_x - (coords.right = coords.left + coords.right + 10) - 10, 20, m_hwnd, (HMENU)ED_XZ, g_hinst, NULL);
        hSt_xz_text_fisher = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD, coords.left, coords.top += 30, coords.right - coords.left - 10, 30, m_hwnd, (HMENU)ST_XZ_TEXT_FISHER, g_hinst, NULL);
        SetWindowText(hSt_xz_text_fisher, TEXT("Критерий Фишера:"));
        hSt_xz_fisher = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD, coords.right, coords.top, right_x - coords.right - 10, 20, m_hwnd, (HMENU)ST_XZ_FISHER, g_hinst, NULL);
        hSt_xz_text_cor = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD, coords.left, coords.top += 30, coords.right - coords.left - 10, 30, m_hwnd, (HMENU)ST_XZ_TEXT_COR, g_hinst, NULL);
        SetWindowText(hSt_xz_text_cor, TEXT("Коэф. парной\nкорреляции:"));
        hSt_xz_cor = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD, coords.right, coords.top + 7, right_x - coords.right - 10, 20, m_hwnd, (HMENU)ST_XZ_COR, g_hinst, NULL);
        SetWindowText(hSt_xz_cor, TEXT("-"));
        SetWindowText(hSt_xz_fisher, TEXT("-"));

        hSt_yz_text_comp = CreateWindow(WC_STATIC, NULL, SS_CENTER | WS_CHILD, coords.left, coords.top += 50, right_x - coords.left - 10, 20, m_hwnd, (HMENU)ST_YZ_TEXT_COMP, g_hinst, NULL);
        SetWindowText(hSt_yz_text_comp, TEXT("Величины Y и Z:"));
        hSt_yz_text_level = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD, coords.left, coords.top += 30, coords.right = 150, 20, m_hwnd, (HMENU)ST_YZ_TEXT_LEVEL, g_hinst, NULL);
        SetWindowText(hSt_yz_text_level, TEXT("Уровень значимости:"));
        hEdit_yz = CreateWindow(WC_EDIT, NULL, WS_BORDER | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL, coords.right, coords.top, right_x - (coords.right = coords.left + coords.right + 10) - 10, 20, m_hwnd, (HMENU)ED_YZ, g_hinst, NULL);
        hSt_yz_text_fisher = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD, coords.left, coords.top += 30, coords.right - coords.left - 10, 30, m_hwnd, (HMENU)ST_YZ_TEXT_FISHER, g_hinst, NULL);
        SetWindowText(hSt_yz_text_fisher, TEXT("Критерий Фишера:"));
        hSt_yz_fisher = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD, coords.right, coords.top, right_x - coords.right - 10, 20, m_hwnd, (HMENU)ST_YZ_FISHER, g_hinst, NULL);
        hSt_yz_text_cor = CreateWindow(WC_STATIC, NULL, SS_LEFT | WS_CHILD, coords.left, coords.top += 30, coords.right - coords.left - 10, 30, m_hwnd, (HMENU)ST_YZ_TEXT_COR, g_hinst, NULL);
        SetWindowText(hSt_yz_text_cor, TEXT("Коэф. парной\nкорреляции:"));
        hSt_yz_cor = CreateWindow(WC_STATIC, NULL, SS_RIGHT | WS_CHILD, coords.right, coords.top + 7, right_x - coords.right - 10, 20, m_hwnd, (HMENU)ST_YZ_COR, g_hinst, NULL);
        SetWindowText(hSt_yz_cor, TEXT("-"));
        SetWindowText(hSt_yz_fisher, TEXT("-"));

        hSt_matrix = CreateWindow(WC_STATIC, NULL, SS_CENTER | WS_CHILD | WS_CLIPSIBLINGS, coords.left, coords.top += 50, right_x - coords.left - 10, 20, m_hwnd, (HMENU)ST_MATRIX, g_hinst, NULL);
        SetWindowText(hSt_matrix, TEXT("Матрица коэф-ов парных корреляций:"));  
        mw.Create(g_hinst, TEXT("Matrix"), WS_CHILD | WS_DLGFRAME, NULL, left_x + 20, coords.top, right_x - left_x - 40, (coords.bottom - 10 - (coords.top += 30)), m_hwnd);
        DeleteObject((HBRUSH)SetClassLongPtr(mw.Window(), GCLP_HBRBACKGROUND, (LONG_PTR)GetSysColorBrush(COLOR_WINDOW)));

        // Приоритет видимости вкладок и инициализация подсказок
        SetWindowPos(hwndTab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        for (uint8_t i(0); i < ARRAYSIZE(m_tip_manager); ++i)
            m_tip_manager[i].Init(m_hwnd);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkMode(hdc, TRANSPARENT);
        break;
    }
    case WM_NOTIFY:
    {
        switch (reinterpret_cast<LPNMHDR>(lParam)->code)
        {
            case LVN_GETDISPINFO:
            {
                NMLVDISPINFO* plvdi = (NMLVDISPINFO*)lParam;
                if (plvdi->hdr.hwndFrom == hListView)
                {
                    switch (plvdi->item.iSubItem)
                    {
                    case 0:
                        _itot_s(plvdi->item.iItem + 1, plvdi->item.pszText, plvdi->item.cchTextMax, 10);
                        break;
                    case 1:
                        plvdi->item.pszText = strings[plvdi->item.iItem].str;
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    switch (plvdi->item.iSubItem)
                    {
                    case 0:
                        plvdi->item.pszText = (LPWSTR)TEXT("0");
                        break;
                    case 1:
                        plvdi->item.pszText = (LPWSTR)TEXT("1");
                        break;
                    case 2:
                        plvdi->item.pszText = (LPWSTR)TEXT("2");
                        break;
                    default:
                        break;
                    }
                }
                break;
            }
            case TCN_SELCHANGING:
                if (isel == 3) [[unlikely]]
                {
                    ShowWindows(SW_HIDE, hSt_xy_text_comp, hSt_xy_text_level, hEdit_xy, hSt_xy_text_fisher, hSt_xy_fisher, hSt_xy_text_cor, hSt_xy_cor,
                        hSt_xz_text_comp, hSt_xz_text_level, hEdit_xz, hSt_xz_text_fisher, hSt_xz_fisher, hSt_xz_text_cor, hSt_xz_cor,
                        hSt_yz_text_comp, hSt_yz_text_level, hEdit_yz, hSt_yz_text_fisher, hSt_yz_fisher, hSt_yz_text_cor, hSt_yz_cor,
                        hSt_matrix, mw.Window());
                    ShowWindows(SW_SHOW, hBt_add, hBt_gen, hBt_save, hBt_load, hBt_clear, hBt_go, hEd_add, hEd_gen, hEd_gen_from, hEd_gen_to, hEdit_pearson,
                        hSt_text_gen1, hSt_text_gen2, hSt_text_gen3, hSt_mean_text, hSt_dispersion_text, hSt_mean, hSt_dispersion, hSt_asymmetry_text,
                        hSt_excess_text, hSt_asymmetry, hSt_excess, hSt_pearson_result_text, hSt_pearson_result, hStatic_pearson, hListView);
                    break;
                }
                GetWindowText(hEd_add, add_text[isel], FLOAT_TEXT_MAX);
                GetWindowText(hEd_gen, gen_text[isel], FLOAT_TEXT_MAX);
                GetWindowText(hEd_gen_from, from_text[isel], FLOAT_TEXT_MAX);
                GetWindowText(hEd_gen_to, to_text[isel], FLOAT_TEXT_MAX);
                GetWindowText(hEdit_pearson, pearson_a_text[isel], FLOAT_TEXT_MAX);
                GetWindowText(hSt_mean, mean_text[isel], FLOAT_TEXT_MAX);
                GetWindowText(hSt_dispersion, dispersion_text[isel], FLOAT_TEXT_MAX);
                GetWindowText(hSt_asymmetry, asymmetry_text[isel], FLOAT_TEXT_MAX);
                GetWindowText(hSt_excess, excess_text[isel], FLOAT_TEXT_MAX);
                GetWindowText(hSt_pearson_result, pearson_p_text[isel], FLOAT_TEXT_MAX);
                break;
            case TCN_SELCHANGE:
            {
                isel = TabCtrl_GetCurSel(hwndTab);
                if (isel != 3)
                {
                    ListView_DeleteAllItems(hListView);
                    ListView_SetItemCount(hListView, strings.size());
                    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);
                    LVITEM lvI;
                    lvI.pszText = LPSTR_TEXTCALLBACK;
                    lvI.mask = LVIF_TEXT | LVIF_DI_SETITEM;
                    lvI.cchTextMax = FLOAT_TEXT_MAX;
                    for (unsigned long i{ 0ul }; i < strings.size(); ++i)
                    {
                        lvI.iItem = i;
                        lvI.iSubItem = 0;
                        SendMessage(hListView, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&lvI));
                        lvI.iSubItem = 1;
                        SendMessage(hListView, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&lvI));
                    }
                    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
                    RedrawWindow(hListView, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
                    SetWindowText(hEd_add, add_text[isel]);
                    SetWindowText(hEd_gen, gen_text[isel]);
                    SetWindowText(hEd_gen_from, from_text[isel]);
                    SetWindowText(hEd_gen_to, to_text[isel]);
                    SetWindowText(hEdit_pearson, pearson_a_text[isel]);
                    SetWindowText(hSt_mean, mean_text[isel]);
                    SetWindowText(hSt_dispersion, dispersion_text[isel]);
                    SetWindowText(hSt_asymmetry, asymmetry_text[isel]);
                    SetWindowText(hSt_excess, excess_text[isel]);
                    SetWindowText(hSt_pearson_result, pearson_p_text[isel]);
                    if (values.size() >= 1)
                    {
                        EnableWindow(hBt_save, TRUE);
                        EnableWindow(hBt_clear, TRUE);
                    }
                    else
                    {
                        EnableWindow(hBt_save, FALSE);
                        EnableWindow(hBt_clear, FALSE);
                    }
                    if (values.size() >= 2)
                        EnableWindow(hBt_go, TRUE);
                    else
                        EnableWindow(hBt_go, FALSE);
                    chartWindow.pValues = &values;
                }
                else
                {
                    ShowWindows(SW_HIDE, hBt_add, hBt_gen, hBt_save, hBt_load, hBt_clear, hBt_go, hEd_add, hEd_gen, hEd_gen_from, hEd_gen_to, hEdit_pearson,
                        hSt_text_gen1, hSt_text_gen2, hSt_text_gen3, hSt_mean_text, hSt_dispersion_text, hSt_mean, hSt_dispersion, hSt_asymmetry_text,
                        hSt_excess_text, hSt_asymmetry, hSt_excess, hSt_pearson_result_text, hSt_pearson_result, hStatic_pearson, hListView);
                    ShowWindows(SW_SHOW, hSt_xy_text_comp, hSt_xy_text_level, hEdit_xy, hSt_xy_text_fisher, hSt_xy_fisher, hSt_xy_text_cor, hSt_xy_cor,
                        hSt_xz_text_comp, hSt_xz_text_level, hEdit_xz, hSt_xz_text_fisher, hSt_xz_fisher, hSt_xz_text_cor, hSt_xz_cor,
                        hSt_yz_text_comp, hSt_yz_text_level, hEdit_yz, hSt_yz_text_fisher, hSt_yz_fisher, hSt_yz_text_cor, hSt_yz_cor,
                        hSt_matrix, mw.Window());
                }
                RedrawWindow(chartWindow.Window(), NULL, NULL, RDW_INTERNALPAINT | RDW_UPDATENOW | RDW_ALLCHILDREN);
                break;
            }
            default: break;
        }
        break;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDM_ABOUT:
            DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), m_hwnd, &About);
            break;
        case IDM_EXIT:
            DestroyWindow(m_hwnd);
            break;
        case BT_ADD:
        {
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
            {
                if (!Verify(hEd_add)) break;
                TCHAR edit1text[FLOAT_TEXT_MAX];
                Edit_GetText(hEd_add, edit1text, FLOAT_TEXT_MAX);
                values.push_back(_tcstof(edit1text, NULL));
                strings.emplace_back(edit1text);

                LVITEM lvI;
                lvI.pszText = LPSTR_TEXTCALLBACK;
                lvI.mask = LVIF_TEXT | LVIF_DI_SETITEM;
                lvI.cchTextMax = FLOAT_TEXT_MAX;
                lvI.iItem = ListView_GetItemCount(hListView);
                lvI.iSubItem = 0;
                ListView_InsertItem(hListView, &lvI);
                lvI.iSubItem = 1;
                ListView_InsertItem(hListView, &lvI);

                RedrawWindow(hListView, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
                Edit_SetText(hEd_add, 0);
                InvalidateRect(hEd_add, NULL, FALSE);
                if (values.size() >= 1)
                {
                    EnableWindow(hBt_save, TRUE);
                    EnableWindow(hBt_clear, TRUE);
                    if (values.size() >= 2)
                        EnableWindow(hBt_go, TRUE);
                }
                if (actual[isel])
                {
                    actual[isel] = false;
                    Reset();
                    RedrawWindow(chartWindow.Window(), NULL, NULL, RDW_INTERNALPAINT | RDW_UPDATENOW | RDW_ALLCHILDREN);
                }
                break;
            }
            default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            }
            break;
        }
        case BT_GEN:
        {
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
            {
                TCHAR buf[10];
                Edit_GetText(hEd_gen, buf, 10);
                if (_tcscmp(buf, TEXT("")) == 0)
                {
                    MessageBox(NULL, TEXT("Необходимо ввести значение в поле редактирования справа."), TEXT("Недопустимый ввод"), MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
                    break;
                }
                _set_errno(0);
                TCHAR* ptr;
                unsigned long valid = _tcstoul(buf, &ptr, 10);
                if (valid == 0 && _tcscmp(buf, TEXT("0")) != 0 || _tcscmp(ptr, TEXT("\0")) != 0)
                {
                    MessageBox(NULL, TEXT("Введено некорректное значение."), TEXT("Недопустимый ввод"), MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
                    break;
                }
                if (errno == ERANGE)
                {
                    MessageBox(NULL, TEXT("Введённое значение вызвало переполнение."), TEXT("Недопустимый ввод"), MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
                    _set_errno(0);
                    break;
                }
                if (!Verify(hEd_gen_from) || !Verify(hEd_gen_to)) break;

                TCHAR edit3text[FLOAT_TEXT_MAX], edit4text[FLOAT_TEXT_MAX];
                Edit_GetText(hEd_gen_from, edit3text, FLOAT_TEXT_MAX);
                Edit_GetText(hEd_gen_to, edit4text, FLOAT_TEXT_MAX);
                float edit3value = _tcstof(edit3text, NULL), edit4value = _tcstof(edit4text, NULL);
                if (edit4value <= edit3value)
                {
                    MessageBox(NULL, TEXT("Верхняя граница случайной величины должна быть больше нижней."), TEXT("Недопустимый ввод"), MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
                    break;
                }
                float mean = (edit4value + edit3value) / 2;
                float stddev = (edit4value - edit3value) / 6;
                random.param(std::normal_distribution<float>::param_type{ mean, stddev });

                ListView_DeleteAllItems(hListView);
                ListView_SetItemCount(hListView, valid);
                SendMessage(hListView, WM_SETREDRAW, FALSE, 0);
                values.clear();
                strings.clear();
                LVITEM lvI;
                lvI.pszText = LPSTR_TEXTCALLBACK;
                lvI.mask = LVIF_TEXT | LVIF_DI_SETITEM;
                lvI.cchTextMax = FLOAT_TEXT_MAX;
                int item = ListView_GetItemCount(hListView);
                for (unsigned long i{ 0ul }; i < valid; ++i, ++item)
                {
                    float temp = random(gen);
                    TCHAR buf[FLOAT_TEXT_MAX];
                    _stprintf_s(buf, FLOAT_TEXT_MAX, TEXT("%f"), temp);
                    values.push_back(temp);
                    strings.emplace_back(buf);
                    lvI.iItem = item;
                    lvI.iSubItem = 0;
                    SendMessage(hListView, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&lvI));
                    lvI.iSubItem = 1;
                    SendMessage(hListView, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&lvI));
                }
                SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
                RedrawWindow(hListView, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
                if (values.size() >= 1)
                {
                    EnableWindow(hBt_save, TRUE);
                    EnableWindow(hBt_clear, TRUE);
                    if (values.size() >= 2)
                        EnableWindow(hBt_go, TRUE);
                }
                if (actual[isel])
                {
                    actual[isel] = false;
                    Reset();
                    RedrawWindow(chartWindow.Window(), NULL, NULL, RDW_INTERNALPAINT | RDW_UPDATENOW | RDW_ALLCHILDREN);
                }
                break;
            }
            default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            }
            break;
        }
        case BT_LOAD:
        {
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                {
                    WCHAR filename[512] = L"data", file_title[512];
                    OPENFILENAME ofn;
                    memset(&ofn, 0, sizeof(OPENFILENAME));
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = m_hwnd;
                    ofn.lpstrFilter = L"Текстовый файл (*.txt)\0*.txt";
                    ofn.lpstrFile = filename;
                    ofn.nMaxFile = ARRAYSIZE(filename);
                    ofn.lpstrDefExt = L"txt";
                    ofn.Flags = OFN_CREATEPROMPT;
                    if (GetOpenFileName(&ofn) == 0)
                        break;
                    values.clear();
                    strings.clear();
                    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);
                    ListView_DeleteAllItems(hListView);
                    std::wifstream file(filename, std::ios_base::in);
                    TCHAR buf[64], delim[] = L" \t", *tok;
                    LVITEM lvI;
                    lvI.pszText = LPSTR_TEXTCALLBACK;
                    lvI.mask = LVIF_TEXT | LVIF_DI_SETITEM;
                    lvI.cchTextMax = FLOAT_TEXT_MAX;
                    int item = ListView_GetItemCount(hListView);
                    while (file.getline(buf, 64))
                    {
                        _tcstok_s(buf, delim, &tok);
                        strings.emplace_back(_tcstok_s(NULL, delim, &tok));
                        values.push_back(_tstof(strings.back().str));
                        lvI.iItem = item;
                        lvI.iSubItem = 0;
                        SendMessage(hListView, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&lvI));
                        lvI.iSubItem = 1;
                        SendMessage(hListView, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&lvI));
                        ++item;
                    }
                    file.close();
                    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
                    RedrawWindow(hListView, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
                    if (values.size() >= 1)
                    {
                        EnableWindow(hBt_save, TRUE);
                        EnableWindow(hBt_clear, TRUE);
                        if (values.size() >= 2)
                            EnableWindow(hBt_go, TRUE);
                        else
                            EnableWindow(hBt_go, FALSE);
                    }
                    else
                    {
                        EnableWindow(hBt_save, FALSE);
                        EnableWindow(hBt_clear, FALSE);
                        EnableWindow(hBt_go, FALSE);
                    }
                    if (actual[isel])
                    {
                        actual[isel] = false;
                        Reset();
                        RedrawWindow(chartWindow.Window(), NULL, NULL, RDW_INTERNALPAINT | RDW_UPDATENOW | RDW_ALLCHILDREN);
                    }
                    break;
                }
                default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            }
            break;
        }
        case BT_SAVE:
        {
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                {
                    WCHAR filename[512] = L"data", file_title[512];
                    OPENFILENAME ofn;
                    memset(&ofn, 0, sizeof(OPENFILENAME));
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = m_hwnd;
                    ofn.lpstrFilter = L"Текстовый файл (*.txt)\0*.txt";
                    ofn.lpstrFile = filename;
                    ofn.nMaxFile = ARRAYSIZE(filename);
                    ofn.lpstrDefExt = L"txt";
                    ofn.Flags = OFN_OVERWRITEPROMPT;
                    if (GetSaveFileName(&ofn) == 0)
                        break;
                    std::wofstream file(filename, std::ios_base::out);
                    TCHAR buf[10];
                    for (unsigned int i = 0; i < strings.size(); ++i)
                    {
                        _itot_s(i+1, buf, 10, 10);
                        file << buf << "\t\t\t" << strings[i].str << '\n';
                    }
                    file.close();
                    break;
                }
                default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            }
            break;
        }
        case BT_CLEAR:
        {
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
            {
                ListView_DeleteAllItems(hListView);
                values.clear();
                strings.clear();
                UpdateWindow(hListView);
                EnableWindow(hBt_save, FALSE);
                EnableWindow(hBt_clear, FALSE);
                EnableWindow(hBt_go, FALSE);
                if (actual[isel])
                {
                    actual[isel] = false;
                    Reset();
                    RedrawWindow(chartWindow.Window(), NULL, NULL, RDW_INTERNALPAINT | RDW_UPDATENOW | RDW_ALLCHILDREN);
                }
                break;
            }
            default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            }
            break;
        }
        case BT_GO:
        {
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                {
                    if (!actual[isel])
                    {
                        actual[isel] = true;
                        chartWindow.need_redraw[isel] = true;
                        chartWindow.pValues = &values;
                        RedrawWindow(chartWindow.Window(), NULL, NULL, RDW_INTERNALPAINT | RDW_UPDATENOW | RDW_ALLCHILDREN);
                        chartWindow.need_redraw[isel] = false;
                    }
                    break;
                }
                default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            }
            break;
        }
        case ED_PEARSON:
        {
            switch (HIWORD(wParam))
            {
                case EN_KILLFOCUS:
                    TrySetPearsonTip();
                    break;
                default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            }
            break;
        }
        case ED_XY:
        {
            switch (HIWORD(wParam))
            {
                case EN_KILLFOCUS:
                    if (VerifyForTip(fisher_lev[0], hEdit_xy))
                        TrySetFisherTip(1, fisher_lev[0] <= fisher_levels[0]);
                    break;
                default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            }
            break;
        }
        case ED_XZ:
        {
            switch (HIWORD(wParam))
            {
                case EN_KILLFOCUS:
                    if (VerifyForTip(fisher_lev[1], hEdit_xz))
                        TrySetFisherTip(2, fisher_lev[1] <= fisher_levels[1]);
                    break;
                default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            }
            break;
        }
        case ED_YZ:
        {
            switch (HIWORD(wParam))
            {
                case EN_KILLFOCUS:
                    if (VerifyForTip(fisher_lev[2], hEdit_yz))
                        TrySetFisherTip(3, fisher_lev[2] <= fisher_levels[2]);
                    break;
                default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
            }
            break;
        }
        default:
            return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
        }
        break;
    }
    default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

#define m_tip_manager m_tip_manager[isel]

LRESULT CALLBACK Direct2dWindow::MouseHook(int nCode, WPARAM wParam, LPARAM lParam)
{
#define isel d2dw.isel
    if (nCode >= 0)
    {
        MOUSEHOOKSTRUCT* pMouseStruct = (MOUSEHOOKSTRUCT*)lParam;
        if (pMouseStruct)
        {
            switch (wParam)
            {
            case WM_NCLBUTTONDOWN:
            case WM_LBUTTONDOWN:
            {
                HWND clicked = WindowFromPoint(pMouseStruct->pt), current = GetFocus();
                if (current != clicked)
                    SetFocus(NULL);
                break;
            }
            case WM_MOUSEMOVE:
            {
                POINT pt_screen = pMouseStruct->pt;
                POINT pt_client = pt_screen;
                MapWindowPoints(NULL, d2dw.Window(), &pt_client, 1);
                TOOLINFO* last_current = d2dw.m_tip_manager.current_tip;
                if (d2dw.m_tip_manager.tracking_now && !d2dw.m_tip_manager.InTrackingRects(&pt_client))
                {
                    PostMessage(d2dw.m_tip_manager.hwndTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)d2dw.m_tip_manager.current_tip);
                    d2dw.m_tip_manager.tracking_now = false;
                    return CallNextHookEx(NULL, nCode, wParam, lParam);
                }
                if (!d2dw.m_tip_manager.tracking_now && d2dw.m_tip_manager.InTrackingRects(&pt_client))
                {
                    PostMessage(d2dw.m_tip_manager.hwndTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, reinterpret_cast<LPARAM>(reinterpret_cast<LPTOOLINFO>(d2dw.m_tip_manager.current_tip)));
                    d2dw.m_tip_manager.tracking_now = true;
                }
                if (last_current && d2dw.m_tip_manager.current_tip->uId != last_current->uId) [[unlikely]] // Курсор зашёл в область другой подсказки без сброса
                {
                    PostMessage(d2dw.m_tip_manager.hwndTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, reinterpret_cast<LPARAM>(last_current));
                    PostMessage(d2dw.m_tip_manager.hwndTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, reinterpret_cast<LPARAM>(d2dw.m_tip_manager.current_tip));
                }
                d2dw.m_tip_manager.last_x = pt_client.x;
                d2dw.m_tip_manager.last_y = pt_client.y;
                PostMessage(d2dw.m_tip_manager.hwndTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(pt_screen.x + 10, pt_screen.y - 20));
                break;
            }
            default: break;
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
#undef isel
}

LRESULT CALLBACK Direct2dWindow::KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && !(lParam & (1 << 31)))
    {
        HWND current = GetFocus();
        if (wParam == VK_ESCAPE) SetFocus(NULL);
        if (current == d2dw.hEd_add)
        {
            switch (wParam)
            {
                case VK_RETURN:
                    SendMessage(d2dw.hBt_add, BM_CLICK, 0, 0);
                    SetFocus(d2dw.hEd_add);
                    break;
                case VK_DOWN:
                    SetFocus(d2dw.hEd_gen);
                    break;
                case VK_UP:
                    SetFocus(d2dw.hEdit_pearson);
                    break;
                default: break;
            }
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        if (current == d2dw.hEd_gen)
        {
            switch (wParam)
            {
            case VK_RETURN:
                SendMessage(d2dw.hBt_gen, BM_CLICK, 0, 0);
                break;
            case VK_DOWN:
                SetFocus(d2dw.hEd_gen_from);
                break;
            case VK_UP:
                SetFocus(d2dw.hEd_add);
                break;
            default: break;
            }
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        if (current == d2dw.hEd_gen_from)
        {
            switch (wParam)
            {
            case VK_RETURN:
                SendMessage(d2dw.hBt_gen, BM_CLICK, 0, 0);
                break;
            case VK_DOWN:
                SetFocus(d2dw.hEd_gen_to);
                break;
            case VK_UP:
                SetFocus(d2dw.hEd_gen);
                break;
            default: break;
            }
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        if (current == d2dw.hEd_gen_to)
        {
            switch (wParam)
            {
            case VK_RETURN:
                SendMessage(d2dw.hBt_gen, BM_CLICK, 0, 0);
                break;
            case VK_DOWN:
                SetFocus(d2dw.hEdit_pearson);
                break;
            case VK_UP:
                SetFocus(d2dw.hEd_gen_from);
                break;
            default: break;
            }
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        if (current == d2dw.hEdit_pearson)
        {
            switch (wParam)
            {
            case VK_RETURN:
                d2dw.TrySetPearsonTip();
                break;
            case VK_DOWN:
                SetFocus(d2dw.hEd_add);
                break;
            case VK_UP:
                SetFocus(d2dw.hEd_gen_to);
                break;
            default: break;
            }
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        if (current == d2dw.hEdit_xy)
        {
            switch (wParam)
            {
            case VK_RETURN:
                if (VerifyForTip(d2dw.fisher_lev[0], d2dw.hEdit_xy))
                    d2dw.TrySetFisherTip(1, d2dw.fisher_lev[0] <= d2dw.fisher_levels[0]);
                break;
            case VK_DOWN:
                SetFocus(d2dw.hEdit_xz);
                break;
            case VK_UP:
                SetFocus(d2dw.hEdit_yz);
                break;
            default: break;
            }
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        if (current == d2dw.hEdit_xz)
        {
            switch (wParam)
            {
            case VK_RETURN:
                if (VerifyForTip(d2dw.fisher_lev[1], d2dw.hEdit_xz))
                    d2dw.TrySetFisherTip(2, d2dw.fisher_lev[1] <= d2dw.fisher_levels[1]);
                break;
            case VK_DOWN:
                SetFocus(d2dw.hEdit_yz);
                break;
            case VK_UP:
                SetFocus(d2dw.hEdit_xy);
                break;
            default: break;
            }
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        if (current == d2dw.hEdit_yz)
        {
            switch (wParam)
            {
            case VK_RETURN:
                if (VerifyForTip(d2dw.fisher_lev[2], d2dw.hEdit_yz))
                    d2dw.TrySetFisherTip(3, d2dw.fisher_lev[2] <= d2dw.fisher_levels[2]);
                break;
            case VK_DOWN:
                SetFocus(d2dw.hEdit_xy);
                break;
            case VK_UP:
                SetFocus(d2dw.hEdit_xz);
                break;
            default: break;
            }
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void Direct2dWindow::TrySetPearsonTip()
{
    if (VerifyForTip(pearson_a, hEdit_pearson) && pearson_p.has_value())
    {
        TCHAR tip_text[256];
        _tcscpy_s(tip_text, ARRAYSIZE(tip_text), pearson_p >= pearson_a ?
            TEXT("Для заданного уровня значимости величина подчиняется теоретическому нормальному закону в соответствии с критерием Пирсона.") :
            TEXT("Для заданного уровня значимости величина не подчиняется теоретическому нормальному закону в соответствии с критерием Пирсона."));
        RECT r1, r2, r3, r4;
        GetWindowRect(hSt_pearson_result_text, &r1); GetWindowRect(hSt_pearson_result, &r2); GetWindowRect(hEdit_pearson, &r3); GetWindowRect(hStatic_pearson, &r4);
        unsigned int group = m_tip_manager.GROUP_PEARSON;
        //std::erase_if(m_tip_manager.tips, [group](auto& pair) { return pair.second.lParam == group; });
        m_tip_manager.EraseGroup(group);
        m_tip_manager.Add(group, tip_text, std::move(r1), std::move(r2), std::move(r3), std::move(r4));
        SendMessage(d2dw.m_tip_manager.hwndTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)m_tip_manager.current_tip);
        d2dw.m_tip_manager.tracking_now = false;
    }
}

#undef m_tip_manager
#undef values

void Direct2dWindow::TrySetFisherTip(uint8_t r, bool is_equal)
{
    if (fisher_res[r-1].has_value())
    {
        TCHAR tip_text[256];
        RECT r1, r2, r3, r4;
        unsigned int group;
        switch (r)
        {
            case 1:
                _stprintf_s(tip_text, 256, TEXT("Для заданного уровня значимости дисперсии величин %s и %s %s в соответствии с критерием Фишера."),
                    TEXT("X"), TEXT("Y"), is_equal ? TEXT("равны") : TEXT("не равны"));
                GetWindowRect(hSt_xy_text_fisher, &r1); GetWindowRect(hSt_xy_fisher, &r2); GetWindowRect(hSt_xy_text_level, &r3); GetWindowRect(hEdit_xy, &r4);
                group = m_tip_manager[3].GROUP_FISHER_XY;
                break;
            case 2:
                _stprintf_s(tip_text, 256, TEXT("Для заданного уровня значимости дисперсии величин %s и %s %s в соответствии с критерием Фишера."),
                    TEXT("X"), TEXT("Z"), is_equal ? TEXT("равны") : TEXT("не равны"));
                GetWindowRect(hSt_xz_text_fisher, &r1); GetWindowRect(hSt_xz_fisher, &r2); GetWindowRect(hSt_xz_text_level, &r3); GetWindowRect(hEdit_xz, &r4);
                group = m_tip_manager[3].GROUP_FISHER_XZ;
                break;
            case 3:
                _stprintf_s(tip_text, 256, TEXT("Для заданного уровня значимости дисперсии величин %s и %s %s в соответствии с критерием Фишера."),
                    TEXT("Y"), TEXT("Z"), is_equal ? TEXT("равны") : TEXT("не равны"));
                GetWindowRect(hSt_yz_text_fisher, &r1); GetWindowRect(hSt_yz_fisher, &r2); GetWindowRect(hSt_yz_text_level, &r3); GetWindowRect(hEdit_yz, &r4);
                group = m_tip_manager[3].GROUP_FISHER_YZ;
                break;
        }
        m_tip_manager[3].EraseGroup(group);
        m_tip_manager[3].Add(group, tip_text, std::move(r1), std::move(r2), std::move(r3), std::move(r4));
        SendMessage(m_tip_manager[isel].hwndTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)m_tip_manager[isel].current_tip);
        m_tip_manager[isel].tracking_now = false;
    }
}


void Direct2dWindow::Reset()
{
    SetWindowText(hSt_mean, TEXT("-"));
    SetWindowText(hSt_dispersion, TEXT("-"));
    SetWindowText(hSt_asymmetry, TEXT("-"));
    SetWindowText(hSt_excess, TEXT("-"));
    SetWindowText(hSt_pearson_result, TEXT("-"));
    m_tip_manager[isel].EraseGroup(m_tip_manager[isel].GROUP_CHART);
    m_tip_manager[isel].EraseGroup(m_tip_manager[isel].GROUP_TEORETICAL);
    m_tip_manager[isel].EraseGroup(m_tip_manager[isel].GROUP_PEARSON);
    pearson_p.reset();
    switch (isel)
    {
        case 0:
            fisher_res[0].reset();
            fisher_res[1].reset();
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_COR_XY);
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_COR_XZ);
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_FISHER_XY);
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_FISHER_XZ);
            SetWindowText(hSt_xy_cor, TEXT("-"));
            SetWindowText(hSt_xy_fisher, TEXT("-"));
            SetWindowText(hSt_xz_cor, TEXT("-"));
            SetWindowText(hSt_xz_fisher, TEXT("-"));
            SetWindowText(mw.statics[0][1], TEXT("-"));
            SetWindowText(mw.statics[0][2], TEXT("-"));
            break;
        case 1:
            fisher_res[0].reset();
            fisher_res[2].reset();
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_COR_XY);
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_COR_YZ);
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_FISHER_XY);
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_FISHER_YZ);
            SetWindowText(hSt_xy_cor, TEXT("-"));
            SetWindowText(hSt_yz_cor, TEXT("-"));
            SetWindowText(hSt_yz_fisher, TEXT("-"));
            SetWindowText(hSt_xy_fisher, TEXT("-"));
            SetWindowText(mw.statics[0][1], TEXT("-"));
            SetWindowText(mw.statics[1][2], TEXT("-"));
            break;
        case 2:
            fisher_res[1].reset();
            fisher_res[2].reset();
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_COR_XZ);
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_COR_YZ);
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_FISHER_XZ);
            m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_FISHER_YZ);
            SetWindowText(hSt_xz_cor, TEXT("-"));
            SetWindowText(hSt_yz_cor, TEXT("-"));
            SetWindowText(hSt_xz_fisher, TEXT("-"));
            SetWindowText(hSt_yz_fisher, TEXT("-"));
            SetWindowText(mw.statics[0][2], TEXT("-"));
            SetWindowText(mw.statics[1][2], TEXT("-"));
            break;
    }
}


void Direct2dWindow::Recompare()
{
    WCHAR text[FLOAT_TEXT_MAX], text2[FLOAT_TEXT_MAX];
    for (uint8_t i(0); i < ARRAYSIZE(values); ++i)
        if (actual[i] && isel != i)
        {
            TCHAR tip_text[256], cor_info[256];
            RECT r1, r2, r3;
            size_t s = min(values[i].size(), values[isel].size());
            float r = 0;
            for (size_t j(0); j < s; ++j)
                r += (values[isel][j] - means[isel]) * (values[i][j] - means[i]);
            r /= (s - 1) * pow(dispersions[isel], 0.5) * pow(dispersions[i], 0.5);
            _stprintf_s(text, FLOAT_TEXT_MAX, TEXT("%f"), r);
            if (Compare(r, 1)) _tcscpy_s(cor_info, TEXT("Между величинами прямая строгая функциональная связь."));
            else if (r > 0.7) _tcscpy_s(cor_info, TEXT("Между величинами прямая сильная связь."));
            else if (r > 0.5) _tcscpy_s(cor_info, TEXT("Между величинами прямая умеренная связь."));
            else if (r > 0.3) _tcscpy_s(cor_info, TEXT("Между величинами прямая слабая связь."));
            else if (r > -0.3) _tcscpy_s(cor_info, TEXT("Между величинами связь практически отсутствует."));
            else if (r > -0.5) _tcscpy_s(cor_info, TEXT("Между величинами обратная слабая связь."));
            else if (r > -0.7) _tcscpy_s(cor_info, TEXT("Между величинами обратная умеренная связь."));
            else if (Compare(r, -1)) _tcscpy_s(cor_info, TEXT("Между величинами обратная строгая функциональная связь."));
            else _tcscpy_s(cor_info, TEXT("Между величинами обратная сильная связь."));
            float emp, teor_level;
            long double d1, d2;
            if (dispersions[isel] >= dispersions[i])
            {
                d1 = values[isel].size() - 1;
                d2 = values[i].size() - 1;
                emp = dispersions[isel] / dispersions[i];
            }
            else
            {
                d1 = values[i].size() - 1;
                d2 = values[isel].size() - 1;
                emp = dispersions[i] / dispersions[isel];
            }
            _stprintf_s(text2, FLOAT_TEXT_MAX, TEXT("%f"), emp);
            teor_level = 1 - boost::math::cdf(boost::math::fisher_f_distribution<float>(d1, d2), emp);
            /*teor_level = 1 - Simpson(0, emp, [d1, d2](float x) -> long double
                {
                    long double p1 = pow(d1/d2, d1/2);
                    long double p2 = pow(x, d1 / 2 - 1);
                    long double p3 = pow(1 + x*d1/d2, -(d1 + d2)/2);
                    long double p4 = p1*p2*p3;
                    long double pb = std::betal(d1/2, d2/2);
                    return p4 / pb;
                });*/
            switch (isel + i)
            {
            case 1: // X-Y
                m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_COR_XY);
                GetWindowRect(hSt_xy_text_cor, &r1); GetWindowRect(hSt_xy_cor, &r2); GetWindowRect(mw.statics[0][1], &r3);
                _stprintf_s(tip_text, 256, TEXT("Коэффициент парной корреляции величин %s и %s равен %f.\n%s"),
                    TEXT("X"), TEXT("Y"), r, cor_info);
                m_tip_manager[3].Add(m_tip_manager[3].GROUP_COR_XY, tip_text, std::move(r1), std::move(r2), std::move(r3));
                SetWindowText(hSt_xy_cor, text);
                SetWindowText(mw.statics[0][1], text);
                SetWindowText(hSt_xy_fisher, text2);
                fisher_levels[0] = teor_level;
                fisher_res[0] = emp;
                if (fisher_lev[0].has_value()) TrySetFisherTip(1, fisher_lev[0] <= teor_level);
                break;
            case 2: // X-Z
                m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_COR_XZ);
                GetWindowRect(hSt_xz_text_cor, &r1); GetWindowRect(hSt_xz_cor, &r2); GetWindowRect(mw.statics[0][2], &r3);
                _stprintf_s(tip_text, 256, TEXT("Коэффициент парной корреляции величин %s и %s равен %f.\n%s"),
                    TEXT("X"), TEXT("Z"), r, cor_info);
                m_tip_manager[3].Add(m_tip_manager[3].GROUP_COR_XZ, tip_text, std::move(r1), std::move(r2), std::move(r3));
                SetWindowText(hSt_xz_cor, text);
                SetWindowText(mw.statics[0][2], text);
                SetWindowText(hSt_xz_fisher, text2);
                fisher_levels[1] = teor_level;
                fisher_res[1] = emp;
                if (fisher_lev[1].has_value()) TrySetFisherTip(2, fisher_lev[1] <= teor_level);
                break;
            case 3: // Y-Z
                m_tip_manager[3].EraseGroup(m_tip_manager[3].GROUP_COR_YZ);
                GetWindowRect(hSt_yz_text_cor, &r1); GetWindowRect(hSt_yz_cor, &r2); GetWindowRect(mw.statics[1][2], &r3);
                _stprintf_s(tip_text, 256, TEXT("Коэффициент парной корреляции величин %s и %s равен %f.\n%s"),
                    TEXT("Y"), TEXT("Z"), r, cor_info);
                m_tip_manager[3].Add(m_tip_manager[3].GROUP_COR_YZ, tip_text, std::move(r1), std::move(r2), std::move(r3));
                SetWindowText(hSt_yz_cor, text);
                SetWindowText(mw.statics[1][2], text);
                SetWindowText(hSt_yz_fisher, text2);
                fisher_levels[2] = teor_level;
                fisher_res[2] = emp;
                if (fisher_lev[2].has_value()) TrySetFisherTip(3, fisher_lev[2] <= teor_level);
                break;
            }
        }
}

void Direct2dWindow::TipManager::Init(HWND _hwnd_parent)
{
    hwndParent = _hwnd_parent;
    hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, _hwnd_parent, NULL, g_hinst, NULL);
    SendMessage(hwndTip, TTM_SETTITLE, TTI_INFO_LARGE, (LPARAM)TEXT("Информация"));
    SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, 400);
    SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    template_struct = {0};
    template_struct.cbSize = TTTOOLINFO_V1_SIZE;
    template_struct.hwnd = hwndParent;
    template_struct.hinst = g_hinst;
    template_struct.uFlags = TTF_TRACK | TTF_ABSOLUTE;
}

void Direct2dWindow::TipManager::EraseGroup(unsigned int group)
{
    std::erase_if(tips, [group, this](auto& tooltip)
        {
            if (tooltip.lParam == group)
            {
                SendMessage(hwndTip, TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&tooltip));
                return true;
            }
            return false;
        });
    current_tip = nullptr;
}

bool Direct2dWindow::TipManager::InTrackingRects(POINT* pt)
{
    for (auto& t : tips)
        if (PtInRect(&t.rect, *pt)) [[unlikely]]
        {
            current_tip = &t;
            return true;
        }
    return false;
}

#undef pearson_a
#undef pearson_p
#undef fisher_res
#undef fisher_lev
#undef strings