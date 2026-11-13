// PyMfcCommon.h —— 公共头/宏/辅助函数/前向声明（PyMfcXaml 各编译单元共享）
#pragma once

// PyMfcCore.cpp — MFC 原生控件的最小 Python 接口。
//
// 生命周期原则（2026-11-04 重构）：
//   1) 所有 MFC 原生对象（窗口、控件容器）由进程级单例
//      PyMfcStore 持有，生命周期到 CAD 进程结束；
//   2) Python 侧对象只持有裸指针，析构时**不 delete** 原生对象，
//      避免卸载栈内销毁窗口导致的堆损坏（0xC0000374）；
//   3) shutdown() 只做隐藏 + 异步关闭窗口，不在 arxunload 调用栈里销毁窗口；
//   4) 侧栏（CAdUiPaletteSet）不在本库：由 PyArxCore.Ap 提供，本库只认页 HWND。
//
// Python 只操作：Window / Page / PaletteSet 上的控件 API（完整清单见交接文档 12.7 节）：
//   add_button / add_label / add_edit / add_listbox / add_checkbox / add_radio /
//   add_group / add_combo / add_progress / add_slider
//   set_text / get_text / set_visible / get_visible / set_enabled / get_enabled /
//   move / set_focus / set_readonly
//   add_string / clear_strings / count_strings / get_string / delete_string /
//   get_cur_sel / set_cur_sel
//   get_check / set_check / set_range / set_pos / get_pos
//   on_click / on_change / on_select
// 所有 HWND 与消息映射归 C++，Python 不接触 CWnd*。

#include "stdafx.h"
#include "XamlCore.h"
#include "XamlDraw.h"     // 三端共用的"声明 → 绘制命令"层
#include "XamlD2D.h"      // Direct2D 后端（DCRenderTarget，画到 owner-draw 的 HDC 上）
#include "XamlControls.h" // 共享绘制器（树/页签/列表等）
#include "PyMfcSkin.h"    // 原生控件的自绘皮肤（绘制权收回 D2D 层）
#include "controls/XamlMfcHost.h"
#include "controls/XamlMfcControlBase.h"
#include "controls/RegisterPyMfcControls.h"
#include "XamlBindings.h"   // PyMfcCore.xaml 子模块：把 PyMfcXaml 加载器暴露给 Python
#include "XamlPowerPlan.h"  // 配电规划图：建树/布局/XAML/JSON/CSV + 离屏渲染
#include "XamlGeometry.h"   // sCurvePoints：回路 S 曲线采样
#include "XamlSvgScene.h"   // SVG：nanosvg 解析 → XamlCanvas 绘制
#include <algorithm>
#pragma comment(lib, "MfcXamlCore.lib")
#include <dwmapi.h>
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
// 图片装载（StaticBitmap / BitmapButton）：用 WIC 解码 png/jpg 再转成 HBITMAP
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
// 验收取证：savePng 用 WIC 编码，文件流走 SHCreateStreamOnFileW
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 配电图绘制：屏幕自绘画布与离屏出图共用同一套画法。
// 参数 k = 缩放（离屏传 scale），(ox, oy) = 视口偏移（离屏传 0,0）。
// 与 HTML 版 power-plan.html 的 buildGraphString 逐项对齐：纸面 / 回路 / 电缆标注 /
// 卡片（root 深边加粗、terminal 浅蓝底）/ 电压角标 / 三行文字 / 折叠 +N。
// -----------------------------------------------------------------------------
static void drawPowerPlanContent(XamlCanvas& cv, const xamlplan::PowerPlan& plan,
                                 float k, int ox, int oy, const std::wstring& sel,
                                 bool showCable = true, const RECT* clip = nullptr)
{
    if (k <= 0.0f) k = 1.0f;
    // 视口裁剪：几千张卡片时只画落在可见矩形里的，屏幕外的直接跳过
    auto offClip = [&](int x0, int y0, int x1, int y1) {
        if (clip == nullptr) return false;
        return x1 < clip->left || x0 > clip->right || y1 < clip->top || y0 > clip->bottom;
    };
    const std::wstring face = L"Microsoft YaHei";
    auto X = [&](double v) { return ox + static_cast<int>(lroundf(static_cast<float>(v) * k)); };
    auto Y = [&](double v) { return oy + static_cast<int>(lroundf(static_cast<float>(v) * k)); };
    auto PX = [&](double v) { return static_cast<int>(lroundf(static_cast<float>(v) * k)); };
    auto ptOf = [&](double px) {
        const int pt = static_cast<int>(px * k * 0.75 + 0.5);
        return pt > 0 ? pt : 1;
    };
    auto units = [](const std::wstring& s) {
        int u = 0;
        for (wchar_t c : s) u += (c >= 0x2E80) ? 2 : 1;
        return u;
    };

    // 纸面
    const RECT paper = { ox, oy, X(plan.width), Y(plan.height) };
    cv.fillRect(paper, RGB(0xFF, 0xFF, 0xFF));
    cv.strokeRect(paper, RGB(0xCF, 0xD6, 0xDF), 1);

    // 选中联动：父链 + 整棵子树的边高亮（与 HTML 的 applySelectionClasses 一致）
    std::map<std::wstring, size_t> idx;
    std::vector<size_t> up(plan.nodes.size(), (size_t)-1);
    std::vector<std::vector<size_t>> kids(plan.nodes.size());
    for (size_t i = 0; i < plan.nodes.size(); ++i) idx[plan.nodes[i].code] = i;
    for (size_t i = 0; i < plan.nodes.size(); ++i)
    {
        auto it = idx.find(plan.nodes[i].upstream);
        if (it != idx.end() && it->second != i)
        {
            up[i] = it->second;
            kids[it->second].push_back(i);
        }
    }
    std::vector<std::wstring> hiEdges;
    if (!sel.empty())
    {
        auto it = idx.find(sel);
        if (it != idx.end())
        {
            size_t cur = it->second;
            while (up[cur] != (size_t)-1)
            {
                hiEdges.push_back(plan.nodes[up[cur]].code + L">" + plan.nodes[cur].code);
                cur = up[cur];
            }
            std::vector<size_t> stack(1, it->second);
            while (!stack.empty())
            {
                const size_t i = stack.back();
                stack.pop_back();
                for (size_t c : kids[i])
                {
                    hiEdges.push_back(plan.nodes[i].code + L">" + plan.nodes[c].code);
                    stack.push_back(c);
                }
            }
        }
    }
    auto isHot = [&](const std::wstring& key) {
        for (const std::wstring& e : hiEdges) if (e == key) return true;
        return false;
    };

    // 回路：三次贝塞尔 → 折线（纵向=竖直 S，横向=水平 S）；先画线，卡片压上面
    for (const xamlplan::PowerLink& l : plan.links)
    {
        // 用户要求：回路线由"曲线"改成"折线"。布局阶段给出的 4 个点
        // (x1,y1) / (mx,y2 or y1) / (mx,y2) / (x2,y2) 本身就是一条正交折线的
        // 拐点序列，直接拿它们连折线即可，不再做贝塞尔采样（不再抹圆角）。
        std::vector<POINT> poly;
        const double raw[4][2] = { { l.bez[0], l.bez[1] }, { l.bez[2], l.bez[3] },
                                   { l.bez[4], l.bez[5] }, { l.bez[6], l.bez[7] } };
        for (int i = 0; i < 4; ++i)
        {
            const POINT pt = { X(raw[i][0]), Y(raw[i][1]) };
            if (!poly.empty() && poly.back().x == pt.x && poly.back().y == pt.y)
                continue;                          // 控制点重合时去掉重复点
            poly.push_back(pt);
        }
        if (!poly.empty())
        {
            int x0 = poly[0].x, x1 = poly[0].x, y0 = poly[0].y, y1 = poly[0].y;
            for (const POINT& pt : poly)
            {
                if (pt.x < x0) x0 = pt.x; if (pt.x > x1) x1 = pt.x;
                if (pt.y < y0) y0 = pt.y; if (pt.y > y1) y1 = pt.y;
            }
            if (offClip(x0, y0, x1, y1)) continue;
        }

        const bool hot = isHot(l.from + L">" + l.to);        const int lineW = hot ? (std::max)(2, PX(2.6)) : (std::max)(1, PX(2.0));
        // 双电源柜的回路（含备用电源进线）画洋红，其余按默认蓝
        const COLORREF lineClr = hot ? RGB(0xE1, 0x57, 0x59)
            : (l.magenta ? RGB(0xC8, 0x00, 0xB4) : RGB(0x2F, 0x6F, 0xBA));
        if (l.dashed)
        {
            // 备用电源进线：沿折线走虚线（画布没有 dash 样式，自己按步长切段）
            const double dash = PX(6.0) > 1 ? PX(6.0) : 2.0;
            const double gapLen = PX(4.0) > 1 ? PX(4.0) : 2.0;
            double sPos = 0.0;
            for (size_t si = 1; si < poly.size(); ++si)
            {
                const POINT a = poly[si - 1];
                const POINT b = poly[si];
                const double dx = b.x - a.x, dy = b.y - a.y;
                const double len = std::sqrt(dx * dx + dy * dy);
                if (len <= 0.0) continue;
                double t = 0.0;
                while (t < len)
                {
                    const double phase = std::fmod(sPos, dash + gapLen);
                    const double remain = (phase < dash) ? (dash - phase) : (dash + gapLen - phase);
                    double t2 = t + remain;
                    if (t2 > len) t2 = len;
                    if (phase < dash)
                    {
                        const POINT pa = { a.x + static_cast<long>(lround(dx * (t / len))),
                                           a.y + static_cast<long>(lround(dy * (t / len))) };
                        const POINT pb = { a.x + static_cast<long>(lround(dx * (t2 / len))),
                                           a.y + static_cast<long>(lround(dy * (t2 / len))) };
                        cv.strokeLine(pa, pb, lineClr, lineW);
                    }
                    sPos += (t2 - t);
                    t = t2;
                }
            }
        }
        else
        {
            cv.strokePath(poly, lineClr, lineW, false);
        }

        // 连线标注电缆：贴在"进入卡片的那一段线"上。
        // 左→右布局 → 横线在 x2 之前，文字左对齐、与横线共线；
        // 上→下布局 → 竖线在 y2 之前，文字整体旋转 90° 与竖线对齐。
        // 这一段线的长度由布局按标注文字宽度留出来（XamlPowerPlan 的 cableLabelWidth）。
        if (showCable && !l.cable.empty() && 10.0 * k >= 1.5)
        {
            // 折线里进入卡片前的那一段：取 poly 中最后一条与目标点共线的边
            const POINT to = { X(l.x2), Y(l.y2) };
            const POINT prev = poly.size() >= 2 ? poly[poly.size() - 2] : to;
            if (l.vertical)
            {
                // 竖排：以卡片上沿的连接点为支点逆时针转 90°，文字沿竖线向上排
                cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Left");
                POINT pivot = to;
                cv.pushRotate(-90, pivot);
                cv.text(RECT{ pivot.x + PX(6), pivot.y - PX(13), pivot.x + PX(226),
                              pivot.y - PX(1) },
                        l.cable, RGB(0x1F, 0x64, 0xB0), DT_LEFT, ptOf(10.0), face);
                cv.popTransform();
            }
            else
            {
                // 横排：左对齐起于拐点稍右，文字坐在横线"上沿"（基线压线，不穿字）
                const int lx = (std::min)(prev.x, to.x) + PX(6);
                cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Left");
                cv.text(RECT{ lx, to.y - PX(13), lx + PX(220), to.y - PX(1) },
                        l.cable, RGB(0x1F, 0x64, 0xB0), DT_LEFT, ptOf(10.0), face);
            }
        }
    }

    // 卡片：电压角标 → 三行文字 → 折叠标记
    for (size_t i = 0; i < plan.boxes.size() && i < plan.nodes.size(); ++i)
    {
        const xamlplan::PowerBox& b = plan.boxes[i];
        const xamlplan::PowerNode& nd = plan.nodes[i];
        const bool isRoot = (nd.role == L"root");
        const bool isTerm = (nd.role == L"terminal");
        const bool isSel = !sel.empty() && sel == nd.code;

        const RECT brc = { X(b.x), Y(b.y), X(b.x + b.w), Y(b.y + b.h) };
        if (offClip(brc.left, brc.top, brc.right, brc.bottom)) continue;
        const int radius = (std::max)(1, PX(2.0));      // HTML rx=2，随缩放走
        COLORREF border = isRoot ? RGB(0x22, 0x22, 0x22)
                                 : (isTerm ? RGB(0x74, 0x86, 0x9C) : RGB(0x8A, 0x94, 0xA6));
        // 双电源柜：卡片描边与标题用红色，底色淡红，一眼能认出是双电源
        const bool isDual = nd.dual != 0;
        if (isDual) border = RGB(0xE0, 0x24, 0x24);
        if (isSel) border = RGB(0xE1, 0x57, 0x59);
        // HTML 的选中只改描边（stroke 2.2），不改底色
        const COLORREF fill = isTerm ? RGB(0xEA, 0xF2, 0xFB)
                                     : (isDual ? RGB(0xFF, 0xF3, 0xF3) : RGB(0xFF, 0xFF, 0xFF));
        cv.fillRoundRect(brc, fill, border, radius);
        // 缩到很小时（整图 fit 到窗口）只铺色块，不再描边/画文字，省掉几千次 GDI 调用
        const int spanPx = brc.right - brc.left;
        if (spanPx < 10)
            continue;
        cv.strokeRoundRect(brc, border, radius,
                           (std::max)(1, PX(isSel ? 2.2 : (isRoot || isDual ? 2.0 : 1.0))));

        // 电压角标（右上角）：380V 蓝 / 220V 棕 / 中压 teal / 高压 深红
        const xamlplan::VoltageStyle vs = xamlplan::voltStyle(nd.voltage);
        if (!vs.text.empty() && 8.5 * k >= 1.5)
        {
            const int bw = PX(8) + static_cast<int>(units(vs.text) * 4.6 * k);
            const int bh = (std::max)(1, PX(11.0));
            const RECT vrc = { X(b.x + b.w) - bw - PX(4), Y(b.y) + PX(3),
                               X(b.x + b.w) - PX(4), Y(b.y) + PX(3) + bh };
            cv.fillRoundRect(vrc, vs.bg, vs.color, (std::max)(1, PX(2.0)));
            cv.setTextStyle(L"SemiBold", L"Normal", L"NoWrap", L"Center");
            cv.text(vrc, vs.text, vs.color, DT_CENTER, ptOf(8.5), face);
        }

        // 正文两行（用户口径）：设备名称 / 功率·电压等级
        if (nd.code.empty() || 14.0 * k < 1.5)
            continue;
        std::vector<std::wstring> lines;
        lines.push_back(nd.name.empty() ? nd.code : nd.name);
        std::wstring l2;
        {
            wchar_t buf[48] = { 0 };
            if (nd.loadKw > 0.0)
            {
                swprintf_s(buf, L"%.3g kW", nd.loadKw);
                l2 = buf;
            }
        }
        if (!vs.text.empty()) l2 += (l2.empty() ? L"" : L" · ") + vs.text;
        if (!l2.empty()) lines.push_back(l2);
        // 第三行（用户口径）：电缆型号 + 合计长度，如 "YJV22-4×35 · 11 m"
        std::wstring l3 = nd.cable;
        if (nd.length > 0.0)
        {
            wchar_t buf[48] = { 0 };
            swprintf_s(buf, L"%.3g m", nd.length);
            if (!l3.empty()) l3 += L" · ";
            l3 += buf;
        }
        if (!l3.empty()) lines.push_back(l3);

        const int blockH = 16 + 13 * (static_cast<int>(lines.size()) - 1);
        const int top0 = static_cast<int>(lroundf((b.h - blockH) * 0.5f));
        for (size_t li = 0; li < lines.size(); ++li)
        {
            const bool first = (li == 0);
            if (!first && 10.5 * k < 1.5) break;
            const int bx = X(b.x + 3);
            const int bw = X(b.x + b.w - 3) - bx;
            const int top = Y(b.y + top0 + static_cast<int>(li) * 13);
            const int bot = Y(b.y + top0 + static_cast<int>(li) * 13 + 13);
            if (first) cv.setTextStyle(L"SemiBold", L"Normal", L"NoWrap", L"Center");
            else cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
            const COLORREF color = first
                ? (isDual ? RGB(0xC0, 0x1B, 0x1B) : RGB(0x1F, 0x2A, 0x37))
                : RGB(0x1F, 0x64, 0xB0);
            cv.text(RECT{ bx, top, bx + bw, bot }, lines[li], color, DT_CENTER,
                    ptOf(first ? (isRoot ? 14.0 : 13.0) : 10.5), face);
        }

        // 折叠标记 "+ N"：HTML 在卡片右上角右对齐（x = n.x + n.w - 6, y = n.y + 12）
        if (nd.collapsedCount > 0 && 9.0 * k >= 1.5)
        {
            wchar_t buf[32] = { 0 };
            swprintf_s(buf, L"+ %d", nd.collapsedCount);
            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Right");
            cv.text(RECT{ X(b.x + b.w - 46), Y(b.y + 1), X(b.x + b.w - 6), Y(b.y + 14) },
                    buf, RGB(0x8A, 0x94, 0xA6), DT_RIGHT, ptOf(9.0), face);
        }
    }
}

namespace
{
    // ---- 本地 Python 直跑时的 MFC 自举 --------------------------------------
    // MFC 的 CWnd::Create / 子类化都依赖模块态里的 CWinApp。CAD 之类宿主本来就有，
    // 直接用；裸 python.exe 里没有，必须补一个最小 CWinApp 再 AfxWinInit，
    // 否则建窗直接访问空指针崩溃（0xC0000005）。
    class PyMfcStandaloneApp : public CWinApp
    {
    public:
        BOOL InitInstance() override { return TRUE; }
    };

    void ensureMfcApp()
    {
        if (AfxGetApp() != nullptr)
            return;                       // 宿主已提供 CWinApp（CAD 进程内）

        static PyMfcStandaloneApp* app = new PyMfcStandaloneApp();
        UNREFERENCED_PARAMETER(app);
        AfxWinInit(::GetModuleHandleW(nullptr), nullptr, ::GetCommandLineW(), 0);

        // 本地直跑没有宿主进程，进程退出阶段显卡驱动销毁设备会在 atidxx64.dll 里空指针崩溃；
        // 强制 D2D 走 WARP 软件渲染，完全不碰显卡驱动（环境变量未显式设置时才设）。
        if (::GetEnvironmentVariableW(L"PYARX_D2D_RT", nullptr, 0) == 0)
            ::SetEnvironmentVariableW(L"PYARX_D2D_RT", L"software");

        // 富文本控件类来自 Msftedit.dll；CAD 里由 acad 预先加载，本地直跑需自己载入
        ::LoadLibraryW(L"Msftedit.dll");

        // 通用控件类（ListView/Tab/DateTime 等）由宿主注册；本地直跑时自己补
        INITCOMMONCONTROLSEX icc = {};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES |
                    ICC_TREEVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_TAB_CLASSES |
                    ICC_BAR_CLASSES | ICC_UPDOWN_CLASS | ICC_DATE_CLASSES |
                    ICC_USEREX_CLASSES;
        ::InitCommonControlsEx(&icc);
    }

    // 日志目录：PYARX_LOG_DIR 优先，否则 PyMfcCore.pyd 自身所在目录下的 log 子目录
    std::wstring pymfcLogFile(const wchar_t* fileName)
    {
        wchar_t env[1024] = {};
        const DWORD n = ::GetEnvironmentVariableW(L"PYARX_LOG_DIR", env, 1024);
        if (n >= 2)
            return std::wstring(env, n) + L"\\" + fileName;

        wchar_t path[MAX_PATH] = {};
        std::wstring dir;
        if (::GetModuleFileNameW(::GetModuleHandleW(L"PyMfcXaml.pyd"), path, MAX_PATH) != 0)
        {
            dir = path;
            const size_t cut = dir.find_last_of(L"\\/");
            if (cut != std::wstring::npos)
                dir.resize(cut);
        }
        dir += L"\\log";
        ::CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\" + fileName;
    }

    // PyMfcCore.pyd 所在目录（Icon="res/xxx.png" 这类相对路径的基准）
    std::wstring pymfcDir()
    {
        wchar_t path[MAX_PATH] = {};
        std::wstring dir;
        if (::GetModuleFileNameW(::GetModuleHandleW(L"PyMfcXaml.pyd"), path, MAX_PATH) != 0)
        {
            dir = path;
            const size_t cut = dir.find_last_of(L"\\/");
            if (cut != std::wstring::npos)
                dir.resize(cut);
        }
        return dir;
    }

    // 仅当环境变量 PYARX_MFC_DEBUG=1 时输出，便于自动化测试定位窗口状态
    void debugLogf(const wchar_t* fmt, ...)
    {
        wchar_t probe[4] = {};
        if (::GetEnvironmentVariableW(L"PYARX_MFC_DEBUG", probe, 4) == 0)
            return;
        FILE* f = _wfopen(pymfcLogFile(L"pymfc_shutdown.log").c_str(), L"a, ccs=UTF-8");
        if (f == nullptr)
            return;
        va_list args;
        va_start(args, fmt);
        vfwprintf(f, fmt, args);
        va_end(args);
        fputws(L"\n", f);
        fclose(f);
    }

    // 绘制耗时埋点：PYARX_MFC_TIMING=1 时把每次自绘的毫秒数单独记一行，
    // 用来判断"卡顿"是画得慢（一次几十毫秒）还是画得勤（同一个控件被反复重画）。
    bool paintTimingEnabled()
    {
        static int cached = -1;
        if (cached < 0)
        {
            wchar_t v[4] = {};
            cached = (::GetEnvironmentVariableW(L"PYARX_MFC_TIMING", v, 4) >= 1 && v[0] == L'1') ? 1 : 0;
        }
        return cached == 1;
    }

    void paintLogf(const wchar_t* fmt, ...)
    {
        if (!paintTimingEnabled())
            return;
        FILE* f = _wfopen(pymfcLogFile(L"pymfc_paint.log").c_str(), L"a, ccs=UTF-8");
        if (f == nullptr)
            return;
        va_list args;
        va_start(args, fmt);
        vfwprintf(f, fmt, args);
        va_end(args);
        fputws(L"\n", f);
        fclose(f);
    }
}

namespace
{
    constexpr UINT kFirstId = 1000;
    constexpr UINT kLastId = 65000;

    // 同一套“加条目”语义在 ListBox 与 ComboBox 上消息号不同，按控件类名分派
    bool isClassOf(HWND hwnd, const wchar_t* className)
    {
        if (hwnd == nullptr || className == nullptr)
            return false;
        WCHAR cls[64] = {};
        if (::GetClassNameW(hwnd, cls, 64) == 0)
            return false;
        return lstrcmpiW(cls, className) == 0;
    }

    bool isComboBox(HWND hwnd) { return isClassOf(hwnd, L"ComboBox"); }
    bool isProgressBar(HWND hwnd) { return isClassOf(hwnd, L"msctls_progress32"); }
    bool isTrackBar(HWND hwnd) { return isClassOf(hwnd, L"msctls_trackbar32"); }

    void invokePython(py::object& callback)
    {
        if (!callback)
            return;
        py::gil_scoped_acquire gil;
        try
        {
            callback();
        }
        catch (py::error_already_set& e)
        {
            // 回调出错时不能静默：stdout 在 CAD 里被重定向，错误必须落到诊断日志
            debugLogf(L"[callback] Python 回调抛异常：%hs", e.what());
            e.restore();
            PyErr_Print();
        }
        catch (...)
        {
            debugLogf(L"[callback] Python 回调抛未知异常");
            PyErr_Print();
        }
    }
}

namespace
{
    ATOM ensureNativeClass()    {
        static ATOM atom = 0;
        if (atom == 0)
        {
            WNDCLASSEXW wc = {};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = ::DefWindowProcW;
            wc.hInstance = AfxGetInstanceHandle();
            wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            wc.lpszClassName = L"PyMfcNativeWindow";
            atom = ::RegisterClassExW(&wc);
        }
        return atom;
    }
}

// 配电画布鼠标状态机：定义在 PyMfcHost 之后（要用到宿主里的画布状态），
// 这里先给前向声明，供 owner-draw 窗口过程调用。
class PyMfcHost;
bool wicWritePng(const std::wstring& path, int w, int h, void* bgra);
static constexpr const wchar_t* kGridHostProp = L"PyMfcGridHost";
static constexpr const wchar_t* kGridOldProp = L"PyMfcGridOldProc";

// 可编辑表格钩子（定义见 PyMfcPlanCanvasMouse 之前）
LRESULT CALLBACK PyMfcGridProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT PyMfcPlanCanvasMouse(PyMfcHost* host, HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam, bool& handled);


// 滚轮拦截：把顶层窗口的窗口过程换成 PyMfcWheelProc，并登记宿主指针与原过程。
// 定义在 PyMfcProcs.cpp；返回原窗口过程，供未命中注册区域的滚轮原样转回。
WNDPROC installWheelHost(HWND hwnd, void* host);

// 手型（可点控件）/ 爪型（画布，系统光标表里没有 grab 手型，从 res\grab.cur 载入，
// 载不到就退回系统手型）。定义在 PyMfcProcs.cpp。
HCURSOR pyMfcHandCursor();
HCURSOR pyMfcGrabCursor();
// 按住拖动（panning）时的"握住的"光标：res\grab_down.cur，缺失时退回 grab
HCURSOR pyMfcGrabDownCursor();
// 画布按当前状态取光标：拖动中=握住的，静止=grab
HCURSOR pyMfcCanvasCursor(HWND hwnd);
