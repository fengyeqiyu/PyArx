#pragma once
// XamlMfcOrder —— Level（绘图层次）/ TabIndex（Tab 键序号）的共享实现。
//
// 为什么单独抽一份：这三端都要"同一套判定"——
//   * 运行时 PyMfcHost（CAD 内嵌，PyMfcCore.pyd）
//   * 预览器 XamlViewer（ViewerMfcHost / ViewerNativeHost）
//   * 设计器 PyMfcDesigner（表面绘制顺序 + 属性面板）
// 各自写一遍必然漂移，所以把"怎么排序"只定义在这里，三端读同一份规则：
//
//   Level     越大越靠上；同级保持"当前叠放序 / XAML 声明序"，幂等不抖。
//   TabIndex  只决定 Tab 遍历次序，-1/未声明 = 按创建序兜底；与叠放次序解耦。
//
// 编码：本文件 UTF-8 with BOM。

#ifndef XAML_MFC_ORDER_H
#define XAML_MFC_ORDER_H

#include <windows.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace xamlmfc
{
    // 控件窗口属性名：Tab 钩子只拿得到 HWND，读不到 C++ 成员，所以两样都写属性。
    const wchar_t* const kSeqProp = L"PyMfcSeq";
    const wchar_t* const kTabProp = L"PyMfcTabIndex";
    const wchar_t* const kLevelProp = L"PyMfcLevel";

    inline void writeIntProp(HWND h, const wchar_t* name, int value)
    {
        if (h != nullptr)
            ::SetPropW(h, name, reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(value)));
    }

    inline int readIntProp(HWND h, const wchar_t* name, int fallback)
    {
        if (h == nullptr)
            return fallback;
        HANDLE v = ::GetPropW(h, name);
        return v == nullptr ? fallback : static_cast<int>(reinterpret_cast<std::intptr_t>(v));
    }

    // 可停留（Tab 能停在上面）的控件类。Win32 只认 WS_TABSTOP，原始 style 里
    // EDIT/BUTTON/COMBOBOX/LISTBOX 都不带，所以创建完要补这一位，否则 Tab 原地不动。
    inline bool isFocusableClass(HWND h)
    {
        wchar_t cls[64] = {0};
        ::GetClassNameW(h, cls, 64);
        return _wcsicmp(cls, L"EDIT") == 0 || _wcsicmp(cls, L"BUTTON") == 0 ||
               _wcsicmp(cls, L"COMBOBOX") == 0 || _wcsicmp(cls, L"LISTBOX") == 0 ||
               _wcsicmp(cls, L"SysLink") == 0;
    }

    inline void ensureTabStop(HWND h)
    {
        if (h == nullptr || !isFocusableClass(h))
            return;
        LONG_PTR st = ::GetWindowLongPtrW(h, GWL_STYLE);
        if ((st & WS_TABSTOP) != 0)
            return;
        ::SetWindowLongPtrW(h, GWL_STYLE, st | WS_TABSTOP);
    }

    // 一个宿主（父窗）内所有控件的层次/序号账本
    class OrderBook
    {
    public:
        void clear() { m_level.clear(); m_tab.clear(); m_seq.clear(); m_seqNext = 0; }

        // 建控件时立刻定创建序（= XAML 声明顺序）；绝不能在整棵树建完后再按叠放倒推，
        // 那时叠放序已经被 Level 改过了。
        void noteCreated(HWND h, int id)
        {
            if (m_seq.find(id) == m_seq.end())
                m_seq[id] = ++m_seqNext;
            writeIntProp(h, kSeqProp, m_seq[id]);
        }

        int seqOf(int id) const
        {
            auto it = m_seq.find(id);
            return it == m_seq.end() ? 0 : it->second;
        }

        void setLevel(HWND h, int id, int level)
        {
            m_level[id] = level;
            writeIntProp(h, kLevelProp, level);
        }
        int levelOf(int id) const
        {
            auto it = m_level.find(id);
            return it == m_level.end() ? 0 : it->second;
        }

        void setTabIndex(HWND h, int id, int index)
        {
            m_tab[id] = index;
            writeIntProp(h, kTabProp, index);
        }
        int tabIndexOf(int id) const
        {
            auto it = m_tab.find(id);
            return it == m_tab.end() ? -1 : it->second;
        }

        int maxLevel() const
        {
            int mx = 0;
            for (const auto& kv : m_level)
                if (kv.second > mx) mx = kv.second;
            return mx;
        }
        int minLevel() const
        {
            int mn = 0;
            for (const auto& kv : m_level)
                if (kv.second < mn) mn = kv.second;
            return mn;
        }
        void bringToFront(HWND h, int id) { setLevel(h, id, maxLevel() + 1); }
        void sendToBack(HWND h, int id) { setLevel(h, id, minLevel() - 1); }

        // 按 (Level, 当前叠放序) 升序逐个 HWND_TOP，最后一个最高；同级保持相对次序。
        void applyZOrder(HWND parent) const
        {
            if (parent == nullptr)
                return;
            struct ZItem
            {
                int level;
                int seq;
                HWND h;
            };
            std::vector<ZItem> items;
            int seq = 0;
            for (HWND h = ::GetWindow(parent, GW_CHILD); h != nullptr;
                 h = ::GetWindow(h, GW_HWNDNEXT))
                items.push_back(ZItem{levelOf(::GetDlgCtrlID(h)), ++seq, h});

            std::sort(items.begin(), items.end(), [](const ZItem& a, const ZItem& b) {
                if (a.level != b.level)
                    return a.level < b.level;
                return a.seq < b.seq;
            });
            for (const ZItem& z : items)
            {
                // Tab 钩子只拿得到 HWND，创建序与 Tab 序号都要落到窗口属性上
                ensureTabStop(z.h);
                writeIntProp(z.h, kSeqProp, seqOf(::GetDlgCtrlID(z.h)));
                writeIntProp(z.h, kTabProp, tabIndexOf(::GetDlgCtrlID(z.h)));
                ::SetWindowPos(z.h, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }

            // 换叠放次序必须整片重画：从底下翻上来的那块，原来压着它的兄弟留下的像素
            // 不会自己消失，看上去就像"层次没生效"。
            ::RedrawWindow(parent, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
        }

        // 父窗里可停控件按 (TabIndex, 创建序) 排；返回 from 的前一个/后一个，绕回。
        HWND nextFocus(HWND parent, HWND from, bool back) const
        {
            if (parent == nullptr)
                return nullptr;
            struct FItem
            {
                int tab;
                int seq;
                HWND h;
            };
            std::vector<FItem> items;
            for (HWND h = ::GetWindow(parent, GW_CHILD); h != nullptr;
                 h = ::GetWindow(h, GW_HWNDNEXT))
            {
                const LONG_PTR st = ::GetWindowLongPtrW(h, GWL_STYLE);
                if ((st & WS_TABSTOP) == 0)
                    continue;
                if (!::IsWindowVisible(h) || !::IsWindowEnabled(h))
                    continue;
                const int id = ::GetDlgCtrlID(h);
                items.push_back(FItem{tabIndexOf(id) >= 0 ? tabIndexOf(id) : 0x7FFFFFFF,
                                      readIntProp(h, kSeqProp, 0x7FFFFFFF), h});
            }
            if (items.empty())
                return nullptr;

            std::sort(items.begin(), items.end(), [](const FItem& a, const FItem& b) {
                if (a.tab != b.tab)
                    return a.tab < b.tab;
                return a.seq < b.seq;
            });

            int cur = -1;
            for (int i = 0; i < static_cast<int>(items.size()); ++i)
                if (items[i].h == from)
                {
                    cur = i;
                    break;
                }
            const int n = static_cast<int>(items.size());
            if (cur < 0)
                return back ? items[n - 1].h : items[0].h;
            const int next = back ? (cur - 1 + n) % n : (cur + 1) % n;
            return items[next].h;
        }

    private:
        std::map<int, int> m_level;   // id -> Level
        std::map<int, int> m_tab;     // id -> TabIndex
        std::map<int, int> m_seq;     // id -> 创建序（= XAML 声明顺序）
        int m_seqNext = 0;
    };

    // ---- Tab 消息钩子：合成/真实 Tab 都走这里 ----------------
    // 三端各自装一次；同一线程只装一个，多个宿主注册进来共用。
    inline std::map<HWND, const OrderBook*>& hookRegistry()
    {
        static std::map<HWND, const OrderBook*> reg;
        return reg;
    }

    inline HWND hookParentOf(HWND focus)
    {
        if (focus == nullptr)
            return nullptr;
        return ::GetParent(focus);
    }

    inline LRESULT CALLBACK orderTabProc(int code, WPARAM wp, LPARAM lp)
    {
        if (code == HC_ACTION && wp == PM_REMOVE)
        {
            MSG* m = reinterpret_cast<MSG*>(lp);
            if (m != nullptr && m->message == WM_KEYDOWN && m->wParam == VK_TAB &&
                (::GetKeyState(VK_CONTROL) & 0x8000) == 0 && (::GetKeyState(VK_MENU) & 0x8000) == 0)
            {
                HWND parent = hookParentOf(::GetFocus());
                auto it = hookRegistry().find(parent);
                if (it != hookRegistry().end() && it->second != nullptr)
                {
                    // Shift 必须读全局键状态：合成输入（keybd_event）只改全局状态，
                    // 本线程的 GetKeyState 看不到，反向 Tab 会被判成正向。
                    const bool back = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    HWND next = it->second->nextFocus(parent, ::GetFocus(), back);
                    if (next != nullptr)
                    {
                        ::SetFocus(next);
                        m->message = WM_NULL;   // 吃掉这条 Tab
                    }
                }
            }
        }
        return ::CallNextHookEx(nullptr, code, wp, lp);
    }

    inline void installOrderTabHook(HWND parent, const OrderBook* book)
    {
        if (parent == nullptr || book == nullptr)
            return;
        hookRegistry()[parent] = book;
        static HHOOK hook = nullptr;
        if (hook == nullptr)
            hook = ::SetWindowsHookExW(WH_GETMESSAGE, &orderTabProc, nullptr, ::GetCurrentThreadId());
    }

    inline void uninstallOrderTabHook(HWND parent)
    {
        hookRegistry().erase(parent);
    }

    // ---- 制作期（设计器）：按 Level 决定兄弟的绘制先后 ----------
    // 设计器表面是单表面自绘，没有真实窗口叠放，所以"谁压谁"只能靠画序：
    // Level 大的后画，同级保持声明序。
    struct DrawOrderItem
    {
        int level;
        int index;    // 声明序
    };

    inline std::vector<int> sortDrawOrder(const std::vector<int>& levels)
    {
        std::vector<DrawOrderItem> items;
        for (int i = 0; i < static_cast<int>(levels.size()); ++i)
            items.push_back(DrawOrderItem{levels[i], i});
        std::sort(items.begin(), items.end(), [](const DrawOrderItem& a, const DrawOrderItem& b) {
            if (a.level != b.level)
                return a.level < b.level;
            return a.index < b.index;
        });
        std::vector<int> out;
        out.reserve(items.size());
        for (const DrawOrderItem& it : items)
            out.push_back(it.index);
        return out;
    }
} // namespace xamlmfc

#endif // XAML_MFC_ORDER_H
