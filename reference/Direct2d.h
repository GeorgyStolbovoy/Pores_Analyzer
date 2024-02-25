#pragma once
#include "framework.h"
#include "WinAPI.h"
#include <CommCtrl.h>
#include <Commdlg.h>
#include "BaseWindow.h"
#include "ChartWindow.h"
#include "d2d1.h"
#include <vector>
#include <random>
#include <chrono>

class Direct2dWindow : public BaseWindow<Direct2dWindow>
{
    class MatrixWindow : public BaseWindow<MatrixWindow>
    {
    public:
        MatrixWindow();
        PCWSTR  ClassName() const override { return szWindowClass; }
        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
        HWND statics[3][3];
    };
    struct Charwrapper
    {
        TCHAR str[FLOAT_TEXT_MAX];
        Charwrapper(TCHAR* other, size_t size = FLOAT_TEXT_MAX)
        {
            _tcscpy_s(str, size, other);
        }
    };
    struct TipManager
    {
        HWND hwndTip, hwndParent;
        bool tracking_now{ false };
        int last_x = 0, last_y = 0;
        std::vector<TOOLINFO> tips;
        TOOLINFO template_struct, *current_tip;
        enum {GROUP_TEORETICAL, GROUP_CHART, GROUP_PEARSON, GROUP_COR_XY, GROUP_COR_XZ, GROUP_COR_YZ, GROUP_FISHER_XY, GROUP_FISHER_XZ, GROUP_FISHER_YZ};

        void Init(HWND hwnd_parent);
        template <typename ...RECTS> void Add(unsigned int group, LPTSTR tip_text, RECT&& rect, RECTS&&... rects);
        void EraseGroup(unsigned int group);
        bool InTrackingRects(POINT* pt);

    private:
        unsigned int unique_id{0u};
        unsigned int new_id() { return ++unique_id; }
    };
    ChartWindow chartWindow;
    std::mt19937 gen{ std::random_device{}() };
    std::normal_distribution<float> random{ 0 };
    HHOOK mouse_hook, keyboard_hook;
    template <typename ...HWNDS> void ShowWindows(int flag, const HWND& hwnd, const HWNDS&... hwnds);
    float fisher_levels[3];

public:
    Direct2dWindow();
    ~Direct2dWindow();
    PCWSTR  ClassName() const override { return szWindowClass; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    static LRESULT CALLBACK MouseHook(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam);
    void TrySetPearsonTip();
    void TrySetFisherTip(uint8_t r, bool is_equal);
    void Recompare();
    void Reset();
    bool Verify(HWND);
    HWND statics[3][3];
    HWND hBt_add, hBt_gen, hBt_save, hBt_load, hBt_clear, hBt_go, hEd_add, hEd_gen, hEd_gen_from, hEd_gen_to, hEdit_pearson,
        hSt_text_gen1, hSt_text_gen2, hSt_text_gen3, hSt_mean_text, hSt_dispersion_text, hSt_mean, hSt_dispersion, hSt_asymmetry_text,
        hSt_excess_text, hSt_asymmetry, hSt_excess, hSt_pearson_result_text, hSt_pearson_result, hStatic_pearson, hListView, hwndTab,
        hSt_xy_text_comp, hSt_xy_text_level, hEdit_xy, hSt_xy_text_fisher, hSt_xy_fisher, hSt_xy_text_cor, hSt_xy_cor,
        hSt_xz_text_comp, hSt_xz_text_level, hEdit_xz, hSt_xz_text_fisher, hSt_xz_fisher, hSt_xz_text_cor, hSt_xz_cor,
        hSt_yz_text_comp, hSt_yz_text_level, hEdit_yz, hSt_yz_text_fisher, hSt_yz_fisher, hSt_yz_text_cor, hSt_yz_cor,
        hSt_matrix;
    enum Controls_ID {BT_ADD = 2000, BT_GEN, BT_SAVE, BT_LOAD, BT_CLEAR, BT_GO, ST_TEXT_GEN_1, ST_TEXT_GEN_2, ST_TEXT_GEN_3, ST_MEAN_TEXT,
        ST_DISPERSION_TEXT, ST_MEAN, ST_DISPERSION, ST_ASYMMETRY_TEXT, ST_EXCESS_TEXT, ST_ASYMMETRY, ST_EXCESS, ST_PEARSON_RESULT_TEXT,
        ST_PEARSON_RESULT, ST_PEARSON, ED_ADD, ED_GEN, ED_GEN_FROM, ED_GEN_TO, ED_PEARSON, LW1, ST_XY_TEXT_COMP, ST_XY_TEXT_LEVEL,
        ED_XY, ST_XY_TEXT_FISHER, ST_XY_FISHER, ST_XY_TEXT_COR, ST_XY_COR, ST_XZ_TEXT_COMP, ST_XZ_TEXT_LEVEL, ED_XZ,
        ST_XZ_TEXT_FISHER, ST_XZ_FISHER, ST_XZ_TEXT_COR, ST_XZ_COR, ST_YZ_TEXT_COMP, ST_YZ_TEXT_LEVEL, ED_YZ,
        ST_YZ_TEXT_FISHER, ST_YZ_FISHER, ST_YZ_TEXT_COR, ST_YZ_COR, ST_MATRIX};
#define TXT(name) name##_text[3][FLOAT_TEXT_MAX]{ TEXT("\0"), TEXT("\0"), TEXT("\0")}
    TCHAR TXT(add), TXT(gen), TXT(from), TXT(to), TXT(pearson_a), TXT(mean), TXT(dispersion), TXT(asymmetry), TXT(excess), TXT(pearson_p);
#undef TXT
    int isel;
    bool actual[3]{false, false, false};
    float means[3];
    float dispersions[3];
    std::vector<float> values[3];
    std::vector<Charwrapper> strings[3];
    std::optional<float> pearson_p[3], pearson_a[3], fisher_res[3], fisher_lev[3];
    TipManager m_tip_manager[4];
    MatrixWindow mw;
};

template <typename ...RECTS>
void Direct2dWindow::TipManager::Add(unsigned int group, LPTSTR tip_text, RECT&& rect, RECTS&&... rects)
{
    unsigned int uId = new_id();
    MapWindowPoints(NULL, hwndParent, reinterpret_cast<LPPOINT>(&rect), 2);
    TOOLINFO toolInfo = template_struct;
    toolInfo.uId = uId;
    toolInfo.rect = rect;
    toolInfo.lpszText = tip_text;
    toolInfo.lParam = group;
    SendMessage(hwndTip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));
    long co = SendMessage(hwndTip, TTM_GETTOOLCOUNT, 0, 0);
    tips.push_back(std::move(toolInfo));
    if constexpr (sizeof...(rects) > 0)
        Add(group, tip_text, std::move(rects)...);
}

template <typename ...HWNDS>
void Direct2dWindow::ShowWindows(int flag, const HWND& hwnd, const HWNDS&... hwnds)
{
    ShowWindow(hwnd, flag);
    if constexpr (sizeof...(hwnds) > 0)
        ShowWindows(flag, hwnds...);
}