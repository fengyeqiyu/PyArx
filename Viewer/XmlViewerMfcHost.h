#pragma once
// XmlViewerMfcHost —— 预览器侧的 MFC 控件宿主。
//
// 预览器里的控件必须是真控件（能打字、能下拉、能勾选），底层用 MFC 控件类
// （CButton / CEdit / CComboBox …）封装，外观走 owner-draw：
// WM_DRAWITEM 里用 D2DDcCanvas 直接画到 DRAWITEMSTRUCT 给的 HDC，
// 绘制函数由外部注入，与设计器自绘、CAD 运行时共用同一份 drawLeaf。
//
// 编码：本文件 UTF-8 with BOM。

#ifndef XMLVIEWER_MFC_HOST_H
#define XMLVIEWER_MFC_HOST_H

// MFC 要求 winsock2.h 先于 windows.h（否则 afxwin.h 直接 #error）
#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif

#include <afxwin.h>
#include <afxcmn.h>
#include <afxdtctl.h>
#include <afxrich.h>

#include <functional>
#include <map>
#include <string>

#include "XmlAppearance.h"
#include "XmlD2D.h"
#include "../MfcXmlCore/PyMfcSkin.h"   // 皮肤绘制（与运行时同一份 drawEdit）
#include "../MfcXmlCore/controls/XmlMfcHost.h"
#include "../MfcXmlCore/controls/XmlMfcOrder.h"

class ViewerMfcHost : public IXmlMfcHost
{
public:
    // 外观回调：宿主只说"这个 id 的控件这一帧要画成什么样"，
    // 具体几何由预览器的 drawLeaf 决定（与设计器同一份绘制代码）。
    typedef std::function<void(XmlCanvas&, int, const RECT&)> DrawFn;

    void init(HWND parent, DrawFn fn)
    {
        m_parent = CWnd::FromHandle(parent);
        m_draw = fn;
        if (m_font.GetSafeHandle() == nullptr)
        {
            m_font.CreateFont(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            m_white.CreateSolidBrush(RGB(255, 255, 255));
            m_page.CreateSolidBrush(RGB(250, 250, 250));
        }
        // Tab 序与叠放序在这里和托管运行时走同一套判定
        xmlmfc::installOrderTabHook(parent, &m_order);
    }

    void destroyAll()
    {
        for (auto& kv : m_items)
        {
            if (kv.second != nullptr)
            {
                if (kv.second->GetSafeHwnd() != nullptr)
                    kv.second->DestroyWindow();
                delete kv.second;
            }
        }
        m_items.clear();
        m_appearances.clear();
        m_checked.clear();
        m_themes.clear();
        if (m_parent != nullptr)
            xmlmfc::uninstallOrderTabHook(m_parent->GetSafeHwnd());
        m_order.clear();
    }

    // ---- IXmlMfcHost：创建 ----
    void addButton(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        CButton* b = make<CButton>(id);
        if (b) b->Create(text.c_str(), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_OWNERDRAW, CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addLabel(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        CStatic* s = make<CStatic>(id);
        if (s) s->Create(text.c_str(), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_OWNERDRAW, CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addEdit(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        CEdit* e = make<CEdit>(id);
        if (e) e->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | ES_AUTOHSCROLL,
                         CRect(x, y, x + w, y + h), m_parent, id);
        setText(id, text);
        applyEditFont(id);      // 控件字体 = 宿主字体，自绘的量字/画字才和光标同源
        attachEditSkin(id);
    }
    void addListBox(int id, int x, int y, int w, int h) override
    {
        CListBox* l = make<CListBox>(id);
        if (l) l->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
                         CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addCheckBox(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        // owner-draw：外观完全由 D2D 画（含勾选方框），状态由宿主维护，
        // 因为没有 BS_AUTOCHECKBOX 的自动切换，点击在 onCommand 里翻转
        m_checked[id] = false;
        CButton* b = make<CButton>(id);
        if (b) b->Create(text.c_str(), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | BS_OWNERDRAW,
                         CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addRadio(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        m_checked[id] = false;
        CButton* b = make<CButton>(id);
        if (b) b->Create(text.c_str(), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | BS_OWNERDRAW,
                         CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addGroup(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        CButton* b = make<CButton>(id);
        if (b) b->Create(text.c_str(), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_GROUPBOX,
                         CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addCombo(int id, int x, int y, int w, int h) override
    {
        addComboAdvanced(id, true, x, y, w, h);
    }
    void addProgress(int id, int x, int y, int w, int h) override
    {
        CProgressCtrl* p = make<CProgressCtrl>(id);
        if (p)
        {
            p->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | PBS_SMOOTH, CRect(x, y, x + w, y + h), m_parent, id);
            p->SetRange(0, 100);
            p->SetPos(0);
        }
    }
    void addSlider(int id, int minValue, int maxValue, int x, int y, int w, int h) override
    {
        CSliderCtrl* s = make<CSliderCtrl>(id);
        if (s)
        {
            s->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBS_AUTOTICKS | TBS_HORZ,
                      CRect(x, y, x + w, y + h), m_parent, id);
            s->SetRange(minValue, maxValue);
            s->SetPos(minValue);
        }
    }
    void addTab(int id, int x, int y, int w, int h) override
    {
        CTabCtrl* t = make<CTabCtrl>(id);
        if (t) t->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addTree(int id, int x, int y, int w, int h) override
    {
        CTreeCtrl* t = make<CTreeCtrl>(id);
        if (t) t->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
                         CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addListCtrl(int id, int x, int y, int w, int h) override
    {
        CListCtrl* l = make<CListCtrl>(id);
        if (l) l->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | LVS_REPORT,
                         CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addChoice(int id, int x, int y, int w, int h) override { addCombo(id, x, y, w, h); }
    void addDatePicker(int id, int x, int y, int w, int h) override
    {
        CDateTimeCtrl* d = make<CDateTimeCtrl>(id);
        if (d) d->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addPanel(int id, int x, int y, int w, int h) override { addStaticPanel(id, x, y, w, h); }
    void addSplitter(int id, int x, int y, int w, int h) override { addStaticPanel(id, x, y, w, h); }
    void addStaticLine(int id, int x, int y, int w, int h) override
    {
        CStatic* s = make<CStatic>(id);
        if (s) s->Create(L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_ETCHEDHORZ, CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addHyperlink(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        CStatic* s = make<CStatic>(id);
        if (s) s->Create(text.c_str(), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_LEFT, CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addRichText(int id, int x, int y, int w, int h) override
    {
        CRichEditCtrl* r = make<CRichEditCtrl>(id);
        if (r) r->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                         CRect(x, y, x + w, y + h), m_parent, id);
        if (r) { applyEditFont(id); attachEditSkin(id); }
    }    void addToggle(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        addCheckBox(id, text, x, y, w, h);
    }
    void addBitmapButton(int id, const std::wstring& text, int x, int y, int w, int h) override
    {
        addButton(id, text, x, y, w, h);
    }
    void addStaticBitmap(int id, int x, int y, int w, int h) override
    {
        CStatic* s = make<CStatic>(id);
        if (s) s->Create(L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_BITMAP | SS_CENTERIMAGE,
                         CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addGrid(int id, int x, int y, int w, int h) override { addListCtrl(id, x, y, w, h); }
    void addScrolledWindow(int id, int x, int y, int w, int h) override
    {
        CStatic* s = make<CStatic>(id);
        if (s) s->Create(L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | WS_VSCROLL | WS_HSCROLL,
                         CRect(x, y, x + w, y + h), m_parent, id);
    }
    void addCalendar(int id, int x, int y, int w, int h) override
    {
        CMonthCalCtrl* c = make<CMonthCalCtrl>(id);
        if (c) c->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, CRect(x, y, x + w, y + h), m_parent, id);
    }

    // ---- 带基础功能的输入类 ----
    void addEditAdvanced(int id, const std::wstring& text, bool multiline, bool readOnly,
                         int maxLength, const std::wstring& placeholder,
                         int x, int y, int w, int h) override
    {
        CEdit* e = make<CEdit>(id);
        if (e == nullptr) return;
        const DWORD style = multiline
                                ? (WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN)
                                : (WS_BORDER | ES_AUTOHSCROLL);
        e->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | style, CRect(x, y, x + w, y + h), m_parent, id);
        setText(id, text);
        if (readOnly) e->SetReadOnly(TRUE);
        if (maxLength > 0) e->SetLimitText((UINT)maxLength);
        applyEditFont(id);
        attachEditSkin(id);   // 输入框也走自绘皮肤，别再让原生 EDIT 画白底 3D 边框
    }

    void addComboAdvanced(int id, bool dropDownList, int x, int y, int w, int h) override
    {
        CComboBox* c = make<CComboBox>(id);
        if (c == nullptr) return;
        // 窗口高度必须含展开区，否则下拉列表被裁掉
        c->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL | (dropDownList ? CBS_DROPDOWNLIST : CBS_DROPDOWN),
                  CRect(x, y, x + w, y + h + 120), m_parent, id);
    }

    void addListBoxAdvanced(int id, bool multipleSelect, int x, int y, int w, int h) override
    {
        CListBox* l = make<CListBox>(id);
        if (l) l->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | LBS_NOTIFY | WS_VSCROLL |
                             (multipleSelect ? LBS_MULTIPLESEL : 0),
                         CRect(x, y, x + w, y + h), m_parent, id);
    }

    void setLimitText(int id, int n) override
    {
        if (HWND h = get(id)) ::SendMessageW(h, EM_SETLIMITTEXT, (WPARAM)(n > 0 ? n : 0), 0);
    }
    void setCueBanner(int id, const std::wstring& text) override
    {
        if (HWND h = get(id)) ::SendMessageW(h, EM_SETCUEBANNER, TRUE, (LPARAM)text.c_str());
    }

    // ---- 属性 ----
    void setText(int id, const std::wstring& text) override
    {
        CWnd* w = item(id);
        if (w == nullptr) return;
        if (w->IsKindOf(RUNTIME_CLASS(CComboBox)))
        {
            CComboBox* c = (CComboBox*)w;
            const int n = c->GetCount();
            for (int i = 0; i < n; ++i)
            {
                CString s;
                c->GetLBText(i, s);
                if (text == (const wchar_t*)s)
                {
                    c->SetCurSel(i);
                    return;
                }
            }
            c->SetWindowText(text.c_str());
            return;
        }
        w->SetWindowText(text.c_str());
    }

    std::wstring getText(int id) const override
    {
        CWnd* w = item(id);
        if (w == nullptr) return {};
        if (w->IsKindOf(RUNTIME_CLASS(CComboBox)))
        {
            CComboBox* c = (CComboBox*)w;
            const int sel = c->GetCurSel();
            if (sel < 0) return {};
            CString s;
            c->GetLBText(sel, s);
            return (const wchar_t*)s;
        }
        CString s;
        w->GetWindowText(s);
        return (const wchar_t*)s;
    }

    void setVisible(int id, bool visible) override
    {
        CWnd* w = item(id);
        if (w) w->ShowWindow(visible ? SW_SHOW : SW_HIDE);
    }
    void setEnabled(int id, bool enabled) override
    {
        CWnd* w = item(id);
        if (w) w->EnableWindow(enabled ? TRUE : FALSE);
    }
    void moveControl(int id, int x, int y, int w, int h) override
    {
        CWnd* win = item(id);
        if (win == nullptr) return;
        int total = h;
        if (win->IsKindOf(RUNTIME_CLASS(CComboBox))) total = h + 120;
        win->MoveWindow(x, y, w, total, TRUE);
    }
    void addString(int id, const std::wstring& text) override
    {
        CWnd* w = item(id);
        if (w == nullptr) return;
        if (w->IsKindOf(RUNTIME_CLASS(CComboBox))) ((CComboBox*)w)->AddString(text.c_str());
        else if (w->IsKindOf(RUNTIME_CLASS(CListBox))) ((CListBox*)w)->AddString(text.c_str());
    }
    // ListCtrl / GridCtrl 的表头与数据行。之前这两个方法没实现（走基类的空实现），
    // XML 里的 Columns / Rows 被静默丢掉，预览器里只剩一个空白列表。
    void addColumn(int id, const std::wstring& title, int width) override
    {
        CWnd* w = item(id);
        if (w == nullptr || !w->IsKindOf(RUNTIME_CLASS(CListCtrl))) return;
        CListCtrl* l = (CListCtrl*)w;
        const int idx = l->GetHeaderCtrl() != nullptr ? l->GetHeaderCtrl()->GetItemCount() : 0;
        l->InsertColumn(idx, title.c_str(), LVCFMT_LEFT, width);
    }
    void addRow(int id, const std::wstring& rowTabJoined) override
    {
        CWnd* w = item(id);
        if (w == nullptr || !w->IsKindOf(RUNTIME_CLASS(CListCtrl))) return;
        CListCtrl* l = (CListCtrl*)w;
        // 行内单元格用 \t 分隔
        int row = l->InsertItem(l->GetItemCount(), L"");
        int col = 0;
        size_t start = 0;
        while (true)
        {
            const size_t tab = rowTabJoined.find(L'\t', start);
            const std::wstring cell = (tab == std::wstring::npos)
                                          ? rowTabJoined.substr(start)
                                          : rowTabJoined.substr(start, tab - start);
            l->SetItemText(row, col, cell.c_str());
            ++col;
            if (tab == std::wstring::npos) break;
            start = tab + 1;
        }
    }
    void clearStrings(int id) override
    {
        CWnd* w = item(id);
        if (w == nullptr) return;
        if (w->IsKindOf(RUNTIME_CLASS(CComboBox))) ((CComboBox*)w)->ResetContent();
        else if (w->IsKindOf(RUNTIME_CLASS(CListBox))) ((CListBox*)w)->ResetContent();
    }
    int countStrings(int id) const override
    {
        CWnd* w = item(id);
        if (w == nullptr) return 0;
        if (w->IsKindOf(RUNTIME_CLASS(CComboBox))) return ((CComboBox*)w)->GetCount();
        if (w->IsKindOf(RUNTIME_CLASS(CListBox))) return ((CListBox*)w)->GetCount();
        return 0;
    }
    std::wstring getString(int id, int index) const override
    {
        CWnd* w = item(id);
        if (w == nullptr) return {};
        CString s;
        if (w->IsKindOf(RUNTIME_CLASS(CComboBox)))
        {
            ((CComboBox*)w)->GetLBText(index, s);
            return (const wchar_t*)s;
        }
        if (w->IsKindOf(RUNTIME_CLASS(CListBox)))
        {
            ((CListBox*)w)->GetText(index, s);
            return (const wchar_t*)s;
        }
        return {};
    }
    void deleteString(int id, int index) override
    {
        CWnd* w = item(id);
        if (w == nullptr) return;
        if (w->IsKindOf(RUNTIME_CLASS(CComboBox))) ((CComboBox*)w)->DeleteString((UINT)index);
        else if (w->IsKindOf(RUNTIME_CLASS(CListBox))) ((CListBox*)w)->DeleteString((UINT)index);
    }
    int getCurSel(int id) const override
    {
        CWnd* w = item(id);
        if (w == nullptr) return -1;
        if (w->IsKindOf(RUNTIME_CLASS(CComboBox))) return ((CComboBox*)w)->GetCurSel();
        if (w->IsKindOf(RUNTIME_CLASS(CListBox))) return ((CListBox*)w)->GetCurSel();
        return -1;
    }
    void setCurSel(int id, int index) override
    {
        CWnd* w = item(id);
        if (w == nullptr) return;
        if (w->IsKindOf(RUNTIME_CLASS(CComboBox))) ((CComboBox*)w)->SetCurSel(index);
        else if (w->IsKindOf(RUNTIME_CLASS(CListBox))) ((CListBox*)w)->SetCurSel(index);
    }
    int getCheck(int id) const override
    {
        auto it = m_checked.find(id);
        return (it != m_checked.end() && it->second) ? 1 : 0;
    }
    void setCheck(int id, bool checked) override
    {
        if (m_checked.find(id) == m_checked.end()) return;
        m_checked[id] = checked;
        if (CWnd* w = item(id)) w->Invalidate();
    }

    // 运行期勾选值的只读访问（绘制回调按它决定画不画勾）
    bool getCheckValue(int id) const
    {
        auto it = m_checked.find(id);
        return it != m_checked.end() && it->second;
    }

    // 父窗口把 WM_COMMAND(BN_CLICKED) 转进来：翻转勾选状态并请求重画
    void onCommand(int id)
    {
        auto it = m_checked.find(id);
        if (it == m_checked.end()) return;
        it->second = !it->second;
        if (CWnd* w = item(id)) w->Invalidate();
    }

    // 单选互斥：同一父容器下只保留一个选中（由预览器在点击后调用）
    void setCheckedExclusive(int id)
    {
        for (auto& kv : m_checked)
            kv.second = (kv.first == id);
        for (auto& kv : m_checked)
            if (CWnd* w = item(kv.first)) w->Invalidate();
    }
    void setRange(int id, int minValue, int maxValue) override
    {
        CWnd* w = item(id);
        if (w == nullptr) return;
        if (w->IsKindOf(RUNTIME_CLASS(CProgressCtrl))) ((CProgressCtrl*)w)->SetRange(minValue, maxValue);
        else if (w->IsKindOf(RUNTIME_CLASS(CSliderCtrl))) ((CSliderCtrl*)w)->SetRange(minValue, maxValue);
    }
    void setPos(int id, int value) override
    {
        CWnd* w = item(id);
        if (w == nullptr) return;
        if (w->IsKindOf(RUNTIME_CLASS(CProgressCtrl))) ((CProgressCtrl*)w)->SetPos(value);
        else if (w->IsKindOf(RUNTIME_CLASS(CSliderCtrl))) ((CSliderCtrl*)w)->SetPos(value);
    }
    int getPos(int id) const override
    {
        CWnd* w = item(id);
        if (w == nullptr) return 0;
        if (w->IsKindOf(RUNTIME_CLASS(CProgressCtrl))) return ((CProgressCtrl*)w)->GetPos();
        if (w->IsKindOf(RUNTIME_CLASS(CSliderCtrl))) return ((CSliderCtrl*)w)->GetPos();
        return 0;
    }
    void setReadOnly(int id, bool readOnly) override
    {
        CWnd* w = item(id);
        if (w != nullptr && w->IsKindOf(RUNTIME_CLASS(CEdit))) ((CEdit*)w)->SetReadOnly(readOnly ? TRUE : FALSE);
    }
    bool getReadOnly(int id) const override
    {
        CWnd* w = item(id);
        if (w != nullptr && w->IsKindOf(RUNTIME_CLASS(CEdit))) return ((CEdit*)w)->GetStyle() & ES_READONLY;
        return false;
    }
    void* getDlgItemHandle(int id) const override
    {
        CWnd* w = item(id);
        return w ? (void*)w->GetSafeHwnd() : nullptr;
    }

    // ---- 通用基础属性：与控制核心库（XmlMfcControlBase）同一套语义 ----
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

    // 真控件的窗口句柄：宿主需要显示/隐藏或挪动某个已经实体化的控件时用它
    HWND ctrlHwnd(int id) const { return get(id); }

    // 控件都建好后统一排一次：边建边排会把还没建出来的兄弟排除在外
    void applyOrder()    {
        if (m_parent != nullptr)
            m_order.applyZOrder(m_parent->GetSafeHwnd());
    }

    // ---- 外观：owner-draw 时由 D2D 画到给定的 HDC ----
    void setControlAppearance(int id, const xmlapp::XmlAppearance& ap) override
    {
        m_appearances[id] = ap;
    }
    void setControlIcon(int, const std::wstring&) override {}
    void addContainerBackdrop(int, int, int, int, const xmlapp::XmlAppearance&) override {}
    void clearContainerBackdrops() override {}

    // 供窗口过程转发：WM_DRAWITEM / WM_CTLCOLOR
    void onDrawItem(LPDRAWITEMSTRUCT lp)
    {
        if (lp == nullptr || m_draw == nullptr) return;
        const int w = (int)(lp->rcItem.right - lp->rcItem.left);
        const int h = (int)(lp->rcItem.bottom - lp->rcItem.top);
        if (w <= 0 || h <= 0) return;

        CDC dc;
        dc.Attach(lp->hDC);
        // 先用页面底色铺满整个控件区域：drawLeaf 只画控件本体，
        // 不铺底会露出按钮窗口的灰底，看起来像一条灰色横带
        const RECT bind = {0, 0, w, h};
        ::FillRect((HDC)dc.GetSafeHdc(), &bind, (HBRUSH)m_page.GetSafeHandle());

        static D2DDcCanvas cv;
        cv.bind((HDC)dc.GetSafeHdc(), bind);
        if (cv.begin(w, h))
        {
            const RECT rc = {0, 0, w, h};
            m_draw(cv, (int)lp->CtlID, rc);
            cv.end();
        }
        else
        {
            ::FillRect((HDC)dc.GetSafeHdc(), &bind, (HBRUSH)m_page.GetSafeHandle());
        }
        dc.Detach();
    }

    HBRUSH onCtlColor(CDC* dc, CWnd* wnd, UINT kind)
    {
        if (dc == nullptr || wnd == nullptr) return nullptr;
        if (kind == CTLCOLOR_EDIT || kind == CTLCOLOR_LISTBOX)
        {
            dc->SetBkColor(RGB(255, 255, 255));
            dc->SetTextColor(RGB(32, 34, 38));
            return (HBRUSH)m_white.m_hObject;
        }
        if (kind == CTLCOLOR_STATIC)
        {
            dc->SetBkMode(TRANSPARENT);
            dc->SetTextColor(RGB(32, 34, 38));
            return (HBRUSH)m_page.m_hObject;
        }
        return nullptr;
    }

    bool isOwnerDrawItem(int id) const { return false; }
    bool hasOwnerDraw(LPDRAWITEMSTRUCT lp) const { (void)lp; return true; }

    // ---- 输入框自绘：接管 WM_PAINT，状态/键盘仍归原生 EDIT ----
    // 原生 EDIT 会自己画白底 + 2px 3D 边框，和其余控件的皮肤画法完全两回事。
    // 这里只把它的 WM_PAINT 掏空，文字/选区/边框全部交给 pymfcskin::drawEdit，
    // 交互（输入、光标、选区、滚动）仍由系统类维护 —— 与运行时同一套分工。
    // 输入框的字体统一取宿主字体：自绘时的量宽（GetTextExtentPoint32W）与画字
    // 必须同一个 HFONT，否则行宽和实际排版对不上，文字会被右端裁掉。
    void applyEditFont(int id)
    {
        HWND h = get(id);
        if (h != nullptr && m_font.GetSafeHandle() != nullptr)
            ::SendMessageW(h, WM_SETFONT,
                           reinterpret_cast<WPARAM>(m_font.GetSafeHandle()), TRUE);
    }

    void attachEditSkin(int id)
    {
        HWND h = get(id);
        if (h == nullptr) return;
        wchar_t cls[64] = {0};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"EDIT") != 0 && _wcsicmp(cls, L"RichEdit20W") != 0 &&
            _wcsicmp(cls, L"RICHEDIT50W") != 0)
            return;
        ::SetPropW(h, kHostProp(), reinterpret_cast<HANDLE>(this));
        WNDPROC old = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtrW(h, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&ViewerMfcHost::editSkinProc)));
        if (old != nullptr) m_editOld[h] = old;
        ::InvalidateRect(h, nullptr, FALSE);
    }

    static const wchar_t* kHostProp() { return L"ViewerMfcHostPtr"; }

    static LRESULT CALLBACK editSkinProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
    {
        ViewerMfcHost* self = reinterpret_cast<ViewerMfcHost*>(::GetPropW(h, kHostProp()));
        if (self == nullptr)
            return ::DefWindowProcW(h, msg, wp, lp);
        auto it = self->m_editOld.find(h);
        WNDPROC old = (it == self->m_editOld.end()) ? nullptr : it->second;

        switch (msg)
        {
        case WM_PAINT:
            self->paintEditSkin(h);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        {
            LRESULT r = old ? ::CallWindowProcW(old, h, msg, wp, lp) : ::DefWindowProcW(h, msg, wp, lp);
            ::InvalidateRect(h, nullptr, FALSE);   // 焦点边框要跟着变
            return r;
        }
        default:
            break;
        }
        return old ? ::CallWindowProcW(old, h, msg, wp, lp) : ::DefWindowProcW(h, msg, wp, lp);
    }

    static int textWidthPx(HDC dc, const std::wstring& s)
    {
        if (s.empty()) return 0;
        SIZE sz = {0, 0};
        ::GetTextExtentPoint32W(dc, s.c_str(), static_cast<int>(s.size()), &sz);
        return sz.cx;
    }

    void paintEditSkin(HWND h)
    {
        PAINTSTRUCT ps;
        HDC hdc = ::BeginPaint(h, &ps);
        if (hdc == nullptr) return;

        RECT rc = {0, 0, 0, 0};
        ::GetClientRect(h, &rc);
        const int w = static_cast<int>(rc.right), hgt = static_cast<int>(rc.bottom);
        if (w <= 0 || hgt <= 0)
        {
            ::EndPaint(h, &ps);
            return;
        }

        const int id = ::GetDlgCtrlID(h);

        // 量宽度要用控件自己的字体：测量归 Win32，绘制归 D2D，字体一致才不会错位
        HDC measure = ::CreateCompatibleDC(hdc);
        HFONT font = reinterpret_cast<HFONT>(::SendMessageW(h, WM_GETFONT, 0, 0));
        if (font != nullptr) ::SelectObject(measure, font);
        TEXTMETRICW tm = {0};
        ::GetTextMetricsW(measure, &tm);
        const int lineH = tm.tmHeight > 0 ? tm.tmHeight : 16;

        const int len = ::GetWindowTextLengthW(h);
        std::wstring text(static_cast<size_t>(len) + 1, L'\0');
        if (len > 0) ::GetWindowTextW(h, text.data(), len + 1);
        text.resize(static_cast<size_t>(len));

        // 选区与光标：EDIT/RichEdit 都吃 EM_GETSEL（字符下标）
        DWORD selStart = 0, selEnd = 0;
        ::SendMessageW(h, EM_GETSEL, reinterpret_cast<WPARAM>(&selStart),
                       reinterpret_cast<LPARAM>(&selEnd));
        const int caretIdx = (selStart == selEnd) ? static_cast<int>(selEnd) : static_cast<int>(selEnd);

        // 按 \n 切行（\r 去掉）：单行控件自然只有一行
        std::vector<std::wstring> rows;
        {
            std::wstring cur;
            for (wchar_t ch : text)
            {
                if (ch == L'\r') continue;
                if (ch == L'\n')
                {
                    rows.push_back(cur);
                    cur.clear();
                }
                else
                    cur.push_back(ch);
            }
            rows.push_back(cur);
        }

        const LONG_PTR style = ::GetWindowLongPtrW(h, GWL_STYLE);
        const bool multiline = (style & ES_MULTILINE) != 0;
        const bool readonly = (style & ES_READONLY) != 0;
        const bool focused = (::GetFocus() == h);

        pymfcskin::Palette pal;
        pal.back = RGB(255, 255, 255);
        pal.fore = RGB(32, 34, 38);
        pal.border = RGB(160, 164, 170);
        pal.accent = RGB(0, 120, 212);
        auto apIt = m_appearances.find(id);
        if (apIt != m_appearances.end())
        {
            COLORREF c = 0;
            if (!apIt->second.backColor.empty() && xmlapp::ParseColor(apIt->second.backColor, c)) pal.back = c;
            if (!apIt->second.foreColor.empty() && xmlapp::ParseColor(apIt->second.foreColor, c)) pal.fore = c;
            if (!apIt->second.borderColor.empty() && xmlapp::ParseColor(apIt->second.borderColor, c)) pal.border = c;
        }

        std::vector<pymfcskin::EditLine> lines;
        int base = 0;   // 当前行首在全文里的下标
        for (size_t i = 0; i < rows.size(); ++i)
        {
            const std::wstring& rowText = rows[i];
            const int lineStart = base;
            const int lineEnd = base + static_cast<int>(rowText.size());

            pymfcskin::EditLine ln;
            ln.text = rowText;
            ln.x = rc.left + 6;
            ln.y = multiline ? (rc.top + 3 + static_cast<int>(i) * lineH)
                             : (rc.top + (hgt - lineH) / 2);

            const int selFrom = static_cast<int>(selStart), selTo = static_cast<int>(selEnd);
            if (selTo > selFrom && selTo > lineStart && selFrom < lineEnd)
            {
                const int from = (selFrom > lineStart) ? selFrom : lineStart;
                const int to = (selTo < lineEnd) ? selTo : lineEnd;
                ln.selFromPx = textWidthPx(measure, rowText.substr(0, static_cast<size_t>(from - lineStart)));
                ln.selToPx = textWidthPx(measure, rowText.substr(0, static_cast<size_t>(to - lineStart)));
            }
            if (caretIdx >= lineStart && caretIdx <= lineEnd)
                ln.caretPx = textWidthPx(measure, rowText.substr(0, static_cast<size_t>(caretIdx - lineStart)));

            lines.push_back(ln);
            base = lineEnd + 1;   // +1 = 被吃掉的换行符
        }

        // 皮肤层照旧用 D2D（与运行时同一份 drawEdit）：底板、边框、选区、文字都在
        // 一次 begin/end 里画完。画布每帧新建 —— DCRenderTarget 的 BindDC 只在建
        // 目标时做一次，复用实例会把新帧画进上一帧已经作废的 DC。
        ::FillRect(hdc, &rc, static_cast<HBRUSH>(m_white.GetSafeHandle()));

        D2DDcCanvas cv;
        cv.bind(hdc, rc);
        if (cv.begin(w, hgt))
        {
            pymfcskin::drawEdit(cv, rc, lines, pal, focused, readonly, lineH, DT_LEFT, focused);
            cv.end();
        }

        ::DeleteDC(measure);
        ::EndPaint(h, &ps);
    }

    std::map<HWND, WNDPROC> m_editOld;   // 被接管的输入框 → 原窗口过程

private:
    void addStaticPanel(int id, int x, int y, int w, int h)
    {
        CStatic* s = make<CStatic>(id);
        if (s) s->Create(L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, CRect(x, y, x + w, y + h), m_parent, id);
    }

    template <class T>
    T* make(int id)
    {
        if (m_parent == nullptr) return nullptr;
        if (CWnd* old = item(id))
        {
            if (old->GetSafeHwnd()) old->DestroyWindow();
            delete old;
            m_items.erase(id);
        }
        T* p = new T();
        m_items[id] = p;
        // 创建序 = XML 声明顺序，建控件当下就定死（不能事后按叠放倒推）
        m_order.noteCreated(nullptr, id);
        return p;
    }

    CWnd* item(int id) const
    {
        auto it = m_items.find(id);
        return it == m_items.end() ? nullptr : it->second;
    }
    HWND get(int id) const
    {
        CWnd* w = item(id);
        return w ? w->GetSafeHwnd() : nullptr;
    }

    CWnd* m_parent = nullptr;
    DrawFn m_draw;
    CFont m_font;
    CBrush m_white;
    CBrush m_page;
    std::map<int, CWnd*> m_items;
    std::map<int, xmlapp::XmlAppearance> m_appearances;
    std::map<int, bool> m_checked;   // owner-draw 复选/单选的运行期状态
    std::map<int, std::wstring> m_themes;
    xmlmfc::OrderBook m_order;      // Level / TabIndex（与运行时同一套判定）
};

#endif // XMLVIEWER_MFC_HOST_H

