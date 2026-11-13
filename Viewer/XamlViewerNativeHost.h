#pragma once
// XamlViewerNativeHost —— 预览器侧的 Win32 原生控件宿主。
//
// 预览器要"能真的打字、能真的下拉"，所以输入类控件必须是真实窗口，
// 而不是 D2D 画出来的一片像素。本宿主实现 IXamlMfcHost（与 CAD 运行时
// PyMfcHost 同一份接口），把每个控件建成真正的 Win32 子窗口。
//
// 编码：本文件 UTF-8 with BOM。

#ifndef XAMLVIEWER_NATIVE_HOST_H
#define XAMLVIEWER_NATIVE_HOST_H

#include <windows.h>
#include <commctrl.h>

#include <map>
#include <string>
#include <vector>

#include "XamlAppearance.h"
#include "../MfcXamlCore/controls/XamlMfcHost.h"
#include "../MfcXamlCore/controls/XamlMfcOrder.h"

class ViewerNativeHost : public IXamlMfcHost
{
public:
    void attachParent(HWND parent)
    {
        m_parent = parent;
        if (m_font == nullptr)
        {
            m_font = ::CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        }
        xamlmfc::installOrderTabHook(parent, &m_order);
    }

    void destroyAll()
    {
        for (auto& kv : m_items)
            if (::IsWindow(kv.second)) ::DestroyWindow(kv.second);
        m_items.clear();
        m_appearances.clear();
        m_themes.clear();
        if (m_parent != nullptr)
            xamlmfc::uninstallOrderTabHook(m_parent);
        m_order.clear();
        if (m_font != nullptr)
        {
            ::DeleteObject(m_font);
            m_font = nullptr;
        }
    }

    // ---- 创建 ----
    void addButton(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        create(L"BUTTON", text, BS_PUSHBUTTON, id, x, y, w, h);
    }
    void addLabel(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        create(L"STATIC", text, SS_LEFT, id, x, y, w, h);
    }
    void addEdit(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        create(L"EDIT", text, WS_BORDER | ES_AUTOHSCROLL, id, x, y, w, h);
    }
    void addListBox(int id, int x, int y, int w, int h) override
    {
        create(L"LISTBOX", L"", WS_BORDER | LBS_NOTIFY | WS_VSCROLL, id, x, y, w, h);
    }
    void addCheckBox(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        create(L"BUTTON", text, BS_AUTOCHECKBOX, id, x, y, w, h);
    }
    void addRadio(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        create(L"BUTTON", text, BS_AUTORADIOBUTTON | WS_GROUP, id, x, y, w, h);
    }
    void addGroup(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        create(L"BUTTON", text, BS_GROUPBOX, id, x, y, w, h);
    }
    void addCombo(int id, int x, int y, int w, int h) override
    {
        // 高度含展开区，否则下拉列表被裁掉
        create(L"COMBOBOX", L"", WS_VSCROLL | CBS_DROPDOWNLIST, id, x, y, w, h + 120);
    }
    void addProgress(int id, int x, int y, int w, int h) override
    {
        create(L"msctls_progress32", L"", PBS_SMOOTH, id, x, y, w, h);
        setRange(id, 0, 100);
        setPos(id, 0);
    }
    void addSlider(int id, int minValue, int maxValue, int x, int y, int w, int h) override
    {
        create(L"msctls_trackbar32", L"", TBS_AUTOTICKS | TBS_HORZ, id, x, y, w, h);
        setRange(id, minValue, maxValue);
        setPos(id, minValue);
    }
    void addTab(int id, int x, int y, int w, int h) override
    {
        create(L"SysTabControl32", L"", WS_TABSTOP, id, x, y, w, h);
    }
    void addTree(int id, int x, int y, int w, int h) override
    {
        create(L"SysTreeView32", L"", WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
               id, x, y, w, h);
    }
    void addListCtrl(int id, int x, int y, int w, int h) override
    {
        create(L"SysListView32", L"", WS_BORDER | LVS_REPORT, id, x, y, w, h);
    }
    void addChoice(int id, int x, int y, int w, int h) override
    {
        create(L"COMBOBOX", L"", WS_VSCROLL | CBS_DROPDOWNLIST, id, x, y, w, h + 120);
    }
    void addDatePicker(int id, int x, int y, int w, int h) override
    {
        create(L"SysDateTimePick32", L"", WS_TABSTOP, id, x, y, w, h);
    }
    void addPanel(int id, int x, int y, int w, int h) override
    {
        create(L"STATIC", L"", 0, id, x, y, w, h);
    }
    void addSplitter(int id, int x, int y, int w, int h) override
    {
        create(L"STATIC", L"", 0, id, x, y, w, h);
    }
    void addStaticLine(int id, int x, int y, int w, int h) override
    {
        create(L"STATIC", L"", SS_ETCHEDHORZ, id, x, y, w, h);
    }
    void addHyperlink(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        create(L"STATIC", text, SS_LEFT, id, x, y, w, h);
    }
    void addRichText(int id, int x, int y, int w, int h) override
    {
        static bool inited = false;
        if (!inited)
        {
            ::LoadLibraryW(L"Msftedit.dll");
            inited = true;
        }
        create(L"RICHEDIT50W", L"", WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
               id, x, y, w, h);
    }
    void addToggle(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        create(L"BUTTON", text, BS_AUTOCHECKBOX | WS_TABSTOP, id, x, y, w, h);
    }
    void addBitmapButton(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        create(L"BUTTON", text, BS_PUSHBUTTON, id, x, y, w, h);
    }
    void addStaticBitmap(int id, int x, int y, int w, int h) override
    {
        create(L"STATIC", L"", SS_BITMAP | SS_CENTERIMAGE, id, x, y, w, h);
    }
    void addGrid(int id, int x, int y, int w, int h) override
    {
        create(L"SysListView32", L"", WS_BORDER | LVS_REPORT, id, x, y, w, h);
    }
    void addScrolledWindow(int id, int x, int y, int w, int h) override
    {
        create(L"STATIC", L"", WS_BORDER | WS_VSCROLL | WS_HSCROLL, id, x, y, w, h);
    }
    void addCalendar(int id, int x, int y, int w, int h) override
    {
        create(L"SysMonthCal32", L"", WS_TABSTOP, id, x, y, w, h);
    }

    // ---- 带基础功能的输入类创建 ----
    void addEditAdvanced(int id, const std::wstring& text, bool multiline, bool readOnly,
                         int maxLength, const std::wstring& placeholder,
                         int x, int y, int w, int h) override
    {
        const DWORD style = multiline
                                ? (WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN)
                                : (WS_BORDER | ES_AUTOHSCROLL);
        create(L"EDIT", text, style, id, x, y, w, h);
        if (readOnly) setReadOnly(id, true);
        if (maxLength > 0) setLimitText(id, maxLength);
        if (!placeholder.empty()) setCueBanner(id, placeholder);
    }

    void addComboAdvanced(int id, bool dropDownList, int x, int y, int w, int h) override
    {
        // CBS_DROPDOWN 可以在下拉项之外手打文字；CBS_DROPDOWNLIST 只能选
        create(L"COMBOBOX", L"", WS_VSCROLL | (dropDownList ? CBS_DROPDOWNLIST : CBS_DROPDOWN),
               id, x, y, w, h + 120);
    }

    void addListBoxAdvanced(int id, bool multipleSelect, int x, int y, int w, int h) override
    {
        create(L"LISTBOX", L"", WS_BORDER | LBS_NOTIFY | WS_VSCROLL |
                                    (multipleSelect ? LBS_MULTIPLESEL : 0),
               id, x, y, w, h);
    }

    // ---- 属性 ----
    void setLimitText(int id, int n) override
    {
        if (HWND h = get(id)) ::SendMessageW(h, EM_SETLIMITTEXT, (WPARAM)(n > 0 ? n : 0), 0);
    }
    void setCueBanner(int id, const std::wstring& text) override
    {
        if (HWND h = get(id)) ::SendMessageW(h, EM_SETCUEBANNER, TRUE, (LPARAM)text.c_str());
    }
    void setText(int id, const std::wstring& text) override
    {
        HWND h = get(id);
        if (!h) return;
        const wchar_t* cls = className(h);
        if (_wcsicmp(cls, L"COMBOBOX") == 0)
        {
            // 下拉框的"当前项"按文本找，找不到就保留原样
            const int n = (int)::SendMessageW(h, CB_GETCOUNT, 0, 0);
            for (int i = 0; i < n; ++i)
            {
                wchar_t buf[512] = {};
                ::SendMessageW(h, CB_GETLBTEXT, (WPARAM)i, (LPARAM)buf);
                if (text == buf)
                {
                    ::SendMessageW(h, CB_SETCURSEL, (WPARAM)i, 0);
                    return;
                }
            }
            if (::GetWindowLongPtrW(h, GWL_STYLE) & CBS_DROPDOWN)
                ::SetWindowTextW(h, text.c_str());
            return;
        }
        ::SetWindowTextW(h, text.c_str());
    }
    std::wstring getText(int id) const override
    {
        HWND h = get(id);
        if (!h) return {};
        if (_wcsicmp(className(h), L"COMBOBOX") == 0)
        {
            const int sel = (int)::SendMessageW(h, CB_GETCURSEL, 0, 0);
            if (sel < 0) return {};
            wchar_t buf[512] = {};
            ::SendMessageW(h, CB_GETLBTEXT, (WPARAM)sel, (LPARAM)buf);
            return buf;
        }
        const int len = ::GetWindowTextLengthW(h);
        std::wstring text((size_t)len + 1, L'\0');
        ::GetWindowTextW(h, text.data(), len + 1);
        text.resize((size_t)len);
        return text;
    }
    void setVisible(int id, bool visible) override
    {
        if (HWND h = get(id)) ::ShowWindow(h, visible ? SW_SHOW : SW_HIDE);
    }
    void setEnabled(int id, bool enabled) override
    {
        if (HWND h = get(id)) ::EnableWindow(h, enabled ? TRUE : FALSE);
    }
    void moveControl(int id, int x, int y, int w, int h) override
    {
        HWND hwnd = get(id);
        if (!hwnd) return;
        LONG_PTR st = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
        const LONG_PTR kind = st & 0xF;
        // 下拉框的窗口高度含下拉列表，界面矩形只是"收起"高度
        int total = h;
        if (_wcsicmp(className(hwnd), L"COMBOBOX") == 0)
            total = h + 120;
        ::MoveWindow(hwnd, x, y, w, total, TRUE);
        (void)kind;
    }
    void addString(int id, const std::wstring& text) override
    {
        if (HWND h = get(id))
            ::SendMessageW(h, isCombo(h) ? CB_ADDSTRING : LB_ADDSTRING, 0, (LPARAM)text.c_str());
    }
    void clearStrings(int id) override
    {
        if (HWND h = get(id))
            ::SendMessageW(h, isCombo(h) ? CB_RESETCONTENT : LB_RESETCONTENT, 0, 0);
    }
    int countStrings(int id) const override
    {
        HWND h = get(id);
        if (!h) return 0;
        return (int)::SendMessageW(h, isCombo(h) ? CB_GETCOUNT : LB_GETCOUNT, 0, 0);
    }
    std::wstring getString(int id, int index) const override
    {
        HWND h = get(id);
        if (!h) return {};
        wchar_t buf[512] = {};
        if (isCombo(h))
            ::SendMessageW(h, CB_GETLBTEXT, (WPARAM)index, (LPARAM)buf);
        else
        {
            wchar_t* p = nullptr;
            if (::SendMessageW(h, LB_GETTEXT, (WPARAM)index, (LPARAM)&p) == LB_ERR || p == nullptr)
                return {};
            return p;
        }
        return buf;
    }
    void deleteString(int id, int index) override
    {
        if (HWND h = get(id))
            ::SendMessageW(h, isCombo(h) ? CB_DELETESTRING : LB_DELETESTRING, (WPARAM)index, 0);
    }
    int getCurSel(int id) const override
    {
        HWND h = get(id);
        if (!h) return -1;
        return (int)::SendMessageW(h, isCombo(h) ? CB_GETCURSEL : LB_GETCURSEL, 0, 0);
    }
    void setCurSel(int id, int index) override
    {
        if (HWND h = get(id))
            ::SendMessageW(h, isCombo(h) ? CB_SETCURSEL : LB_SETCURSEL, (WPARAM)index, 0);
    }
    int getCheck(int id) const override
    {
        HWND h = get(id);
        return h ? (int)::SendMessageW(h, BM_GETCHECK, 0, 0) : 0;
    }
    void setCheck(int id, bool checked) override
    {
        if (HWND h = get(id))
            ::SendMessageW(h, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    void setRange(int id, int minValue, int maxValue) override
    {
        HWND h = get(id);
        if (!h) return;
        if (_wcsicmp(className(h), L"msctls_progress32") == 0)
        {
            ::SendMessageW(h, PBM_SETRANGE32, (WPARAM)minValue, (LPARAM)maxValue);
        }
        else if (_wcsicmp(className(h), L"msctls_trackbar32") == 0)
        {
            ::SendMessageW(h, TBM_SETRANGE, TRUE, MAKELONG(minValue, maxValue));
        }
    }
    void setPos(int id, int value) override
    {
        HWND h = get(id);
        if (!h) return;
        if (_wcsicmp(className(h), L"msctls_progress32") == 0)
            ::SendMessageW(h, PBM_SETPOS, (WPARAM)value, 0);
        else if (_wcsicmp(className(h), L"msctls_trackbar32") == 0)
            ::SendMessageW(h, TBM_SETPOS, TRUE, (LPARAM)value);
    }
    int getPos(int id) const override
    {
        HWND h = get(id);
        if (!h) return 0;
        if (_wcsicmp(className(h), L"msctls_progress32") == 0)
            return (int)::SendMessageW(h, PBM_GETPOS, 0, 0);
        if (_wcsicmp(className(h), L"msctls_trackbar32") == 0)
            return (int)::SendMessageW(h, TBM_GETPOS, 0, 0);
        return 0;
    }
    void setReadOnly(int id, bool readOnly) override
    {
        if (HWND h = get(id)) ::SendMessageW(h, EM_SETREADONLY, readOnly ? TRUE : FALSE, 0);
    }
    bool getReadOnly(int id) const override
    {
        HWND h = get(id);
        if (!h) return false;
        return (::GetWindowLongPtrW(h, GWL_STYLE) & ES_READONLY) != 0;
    }
    void* getDlgItemHandle(int id) const override
    {
        return (void*)get(id);
    }

    // ---- 通用基础属性：与控制核心库（XamlMfcControlBase）同一套语义 ----
    void setControlLevel(int id, int level) override { m_order.setLevel(get(id), id, level); }
    int getControlLevel(int id) const override { return m_order.levelOf(id); }
    void setControlTabIndex(int id, int index) override { m_order.setTabIndex(get(id), id, index); }
    int getControlTabIndex(int id) const override { return m_order.tabIndexOf(id); }
    void setControlTheme(int id, const std::wstring& theme) override { m_themes[id] = theme; }
    std::wstring getControlTheme(int id) const override
    {
        auto it = m_themes.find(id);
        return it == m_themes.end() ? std::wstring() : it->second;
    }
    void applyOrder()
    {
        m_order.applyZOrder(m_parent);
    }

    // ---- 外观（预览器第一版由系统主题绘制，先占位保存） ----
    void setControlAppearance(int id, const xamlapp::XamlAppearance& ap) override
    {
        m_appearances[id] = ap;
        if (HWND h = get(id))
            ::SetWindowTheme(h, L"DarkMode_Explorer", nullptr);
    }
    void setControlIcon(int, const std::wstring&) override {}
    void addContainerBackdrop(int, int, int, int, const xamlapp::XamlAppearance&) override {}
    void clearContainerBackdrops() override {}

    HWND get(int id) const
    {
        auto it = m_items.find(id);
        return it == m_items.end() ? nullptr : it->second;
    }

private:
    static const wchar_t* className(HWND h)
    {
        static wchar_t buf[64] = {};
        ::GetClassNameW(h, buf, 64);
        return buf;
    }
    static bool isCombo(HWND h)
    {
        return _wcsicmp(className(h), L"COMBOBOX") == 0;
    }

    void create(const wchar_t* cls, const std::wstring& text, DWORD style,
                int id, int x, int y, int w, int h)
    {
        if (m_parent == nullptr) return;
        if (HWND old = get(id))
        {
            ::DestroyWindow(old);
            m_items.erase(id);
        }
        HWND hwnd = ::CreateWindowExW(
            0, cls, text.c_str(), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | style,
            x, y, w, h, m_parent, (HMENU)(INT_PTR)id, ::GetModuleHandleW(nullptr), nullptr);
        if (hwnd == nullptr) return;
        if (m_font != nullptr) ::SendMessageW(hwnd, WM_SETFONT, (WPARAM)m_font, TRUE);
        m_items[id] = hwnd;
        m_order.noteCreated(hwnd, id);   // 创建序 = XAML 声明顺序，落地即定
    }

    HWND m_parent = nullptr;
    HFONT m_font = nullptr;
    std::map<int, HWND> m_items;
    std::map<int, xamlapp::XamlAppearance> m_appearances;
    std::map<int, std::wstring> m_themes;
    xamlmfc::OrderBook m_order;
};

#endif // XAMLVIEWER_NATIVE_HOST_H

