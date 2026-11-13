#pragma once
// PyMfcDesigner 工具函数（纯函数，无状态）
#include "stdafx.h"

namespace uidesign
{
    // 本地声明 SHSTOCKICONINFO（避免依赖 shell32.lib，运行时按名字取函数）
    struct SHStockIconInfoLocal
    {
        DWORD cbSize;
        HICON hIcon;
        int iSysImageIndex;
        int iIcon;
        WCHAR szPath[MAX_PATH];
    };
    typedef HRESULT(WINAPI *PFN_SHGetStockIconInfo)(int siid, UINT uFlags, SHStockIconInfoLocal* info);

    inline int StockIconIdForName(const CString& icon)
    {
        if (icon.CompareNoCase(L"open") == 0) return 4;   // SIID_FOLDEROPEN
        if (icon.CompareNoCase(L"save") == 0) return 6;   // SIID_DRIVE35（软盘=保存）
        if (icon.CompareNoCase(L"add") == 0) return 76;  // SIID_FOLDERFRONT
        if (icon.CompareNoCase(L"label") == 0) return 1;   // SIID_DOCASSOC
        if (icon.CompareNoCase(L"ok") == 0) return 79;    // SIID_INFO
        if (icon.CompareNoCase(L"undo") == 0) return 23;  // SIID_UNDO
        if (icon.CompareNoCase(L"redo") == 0) return 24;  // SIID_REDO
        if (icon.CompareNoCase(L"cut") == 0) return 25;   // SIID_CUT
        if (icon.CompareNoCase(L"copy") == 0) return 26; // SIID_COPY
        if (icon.CompareNoCase(L"paste") == 0) return 27;  // SIID_PASTE
        if (icon.CompareNoCase(L"del") == 0) return 13;    // SIID_DELETE
        if (icon.CompareNoCase(L"grid") == 0) return 65; // SIID_PROPERTY
        if (icon.CompareNoCase(L"ruler") == 0) return 0;  // SIID_DOCNOASSOC
        if (icon.CompareNoCase(L"zoomout") == 0) return 29; // SIID_ZOOMOUT
        if (icon.CompareNoCase(L"zoomin") == 0) return 28; // SIID_ZOOMIN
        return -1;
    }

    inline HICON LoadStockIcon16(int siid)
    {
        if (siid < 0) return nullptr;
        static PFN_SHGetStockIconInfo pfn = nullptr;
        static BOOL resolved = FALSE;
        if (!resolved)
        {
            resolved = TRUE;
            HMODULE h = ::GetModuleHandleW(L"shell32.dll");
            if (h == nullptr) h = ::LoadLibraryW(L"shell32.dll");
            if (h != nullptr) pfn = reinterpret_cast<PFN_SHGetStockIconInfo>(::GetProcAddress(h, "SHGetStockIconInfo"));
        }
        if (pfn == nullptr) return nullptr;
        SHStockIconInfoLocal info;
        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        const UINT flags = 0x00000100 | 0x00000001; // SHGSI_ICON | SHGSI_SMALLICON
        HRESULT hr = pfn(siid, flags, &info);
        return SUCCEEDED(hr) ? info.hIcon : nullptr;
    }

    inline COLORREF ParseColor(const CString& s)
    {
        CString v = s;
        v.Trim();
        if (v.IsEmpty()) return (COLORREF)-1;
        if (v[0] == L'#')
        {
            long n = wcstol(v.Mid(1), nullptr, 16);
            return RGB((n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF);
        }
        if (v.Find(L"RGB") == 0 || v.Find(L"rgb") == 0)
        {
            int a, b, c;
            if (swscanf_s(v, L"RGB(%d,%d,%d)", &a, &b, &c) == 3 ||
                swscanf_s(v, L"rgb(%d,%d,%d)", &a, &b, &c) == 3)
                return RGB(a, b, c);
        }
        return (COLORREF)-1;
    }

    inline void DrawRounded(CDC& dc, const CRect& rc, COLORREF fill, COLORREF border, int radius = 6)
    {
        CPen pen(PS_SOLID, 1, border);
        CBrush brush(fill);
        CPen* oldPen = dc.SelectObject(&pen);
        CBrush* oldBrush = dc.SelectObject(&brush);
        dc.RoundRect(rc, CPoint(radius, radius));
        dc.SelectObject(oldBrush);
        dc.SelectObject(oldPen);
    }

    // 绘制控件类型的 16x16 简笔图标（工具栏/控件箱用）
    inline void DrawToolIcon(CDC& dc, const CRect& rc, const CString& tag)
    {
        CPen pen(PS_SOLID, 1, RGB(150, 195, 235));
        CPen* oldPen = dc.SelectObject(&pen);
        CBrush* oldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
        int cx = rc.CenterPoint().x, cy = rc.CenterPoint().y;
        CRect b(cx - 9, cy - 9, cx + 9, cy + 9);
        if (tag == L"Button") dc.RoundRect(b, CPoint(3, 3));
        else if (tag == L"TextBlock" || tag == L"Label")
        {
            dc.MoveTo(cx - 5, cy + 7); dc.LineTo(cx - 5, cy - 7); dc.LineTo(cx + 5, cy - 7);
            dc.MoveTo(cx - 2, cy - 1); dc.LineTo(cx + 2, cy - 1);
        }
        else if (tag == L"TextBox") { dc.Rectangle(b); dc.MoveTo(cx + 4, cy - 6); dc.LineTo(cx + 4, cy + 6); }
        else if (tag == L"CheckBox")
        {
            dc.Rectangle(b);
            dc.MoveTo(cx - 5, cy); dc.LineTo(cx - 1, cy + 4); dc.LineTo(cx + 5, cy - 5);
        }
        else if (tag == L"ComboBox")
        {
            dc.Rectangle(b);
            dc.MoveTo(cx + 2, cy + 2); dc.LineTo(cx + 6, cy + 2); dc.LineTo(cx + 4, cy + 5);
        }
        else if (tag == L"ListBox")
        {
            dc.Rectangle(b);
            for (int i = 0; i < 3; ++i) { dc.MoveTo(cx - 6, cy - 4 + i * 5); dc.LineTo(cx + 6, cy - 4 + i * 5); }
        }
        else if (tag == L"RadioButton") { dc.Ellipse(b); dc.Ellipse(cx - 3, cy - 3, cx + 3, cy + 3); }
        else if (tag == L"Image")
        {
            dc.Rectangle(b);
            dc.MoveTo(cx - 9, cy + 9); dc.LineTo(cx - 2, cy - 1); dc.LineTo(cx + 3, cy + 4); dc.LineTo(cx + 9, cy - 3);
        }
        else if (tag == L"ProgressBar")
        {
            dc.Rectangle(b);
            CRect f(b.left + 2, b.top + 3, b.left + 7, b.bottom - 3);
            dc.FillSolidRect(f, RGB(0, 150, 220));
        }
        else if (tag == L"ListView")
        {
            dc.Rectangle(b);
            dc.MoveTo(cx, cy - 9); dc.LineTo(cx, cy + 9);
            dc.MoveTo(cx - 9, cy); dc.LineTo(cx + 9, cy);
        }
        else if (tag == L"GroupBox")
        {
            dc.Rectangle(b);
            dc.Rectangle(b.left + 2, b.top - 3, b.left + 16, b.top + 3);
        }
        else if (tag == L"TabControl")
        {
            dc.Rectangle(b);
            dc.Rectangle(b.left + 1, b.top - 4, b.left + 10, b.top + 1);
            dc.Rectangle(b.left + 12, b.top - 4, b.left + 19, b.top + 1);
        }
        else if (tag == L"Border") { dc.Rectangle(b); dc.Rectangle(b.left + 2, b.top + 2, b.right - 2, b.bottom - 2); }
        else if (tag == L"Panel") { dc.Rectangle(b); dc.Rectangle(b.left + 3, b.top + 3, b.right - 3, b.bottom - 3); }
        else if (tag == L"SplitContainer")
        {
            dc.Rectangle(b);
            dc.MoveTo(cx, cy - 9); dc.LineTo(cx, cy + 9);
            dc.FillSolidRect(CRect(cx - 2, cy - 2, cx + 2, cy + 2), RGB(150, 195, 235));
        }
        else if (tag == L"MenuBar" || tag == L"MenuStrip")
        {
            dc.Rectangle(b.left - 2, b.top - 4, b.right + 2, b.top + 2);
            for (int i = 0; i < 3; ++i) dc.Rectangle(b.left + i * 6 - 2, b.top - 3, b.left + i * 6 + 2, b.top - 1);
        }
        else if (tag == L"ToolBar" || tag == L"ToolStrip")
        {
            dc.Rectangle(b.left - 2, b.top - 2, b.right + 2, b.top + 4);
            for (int i = 0; i < 3; ++i) dc.Rectangle(b.left + i * 6 - 1, b.top - 1, b.left + i * 6 + 2, b.top + 2);
        }
        else if (tag == L"StatusBar" || tag == L"StatusStrip")
        {
            dc.Rectangle(b.left - 2, b.bottom - 3, b.right + 2, b.bottom + 3);
            dc.MoveTo(cx, b.bottom - 3); dc.LineTo(cx, b.bottom + 3);
        }
        else if (tag == L"Timer" || tag == L"Clock")
        {
            dc.Ellipse(b);
            dc.MoveTo(cx, cy); dc.LineTo(cx, cy - 6);
            dc.MoveTo(cx, cy); dc.LineTo(cx + 4, cy + 1);
        }
        else if (tag == L"PrintDocument")
        {
            dc.Rectangle(b.left + 2, b.top - 6, b.right - 2, b.bottom + 4);
            dc.Rectangle(b.left + 4, b.top + 2, b.right - 4, b.bottom - 2);
        }
        else if (tag == L"OpenFileDialog" || tag == L"SaveFileDialog")
        {
            dc.Rectangle(b);
            dc.MoveTo(cx - 5, cy - 4); dc.LineTo(cx + 5, cy - 4);
            dc.MoveTo(cx - 5, cy); dc.LineTo(cx + 5, cy);
            dc.MoveTo(cx - 5, cy + 4); dc.LineTo(cx + 1, cy + 4);
        }
        else if (tag == L"DataGridView" || tag == L"Grid" || tag == L"GridView")
        {
            dc.Rectangle(b);
            dc.MoveTo(cx - 9, cy - 3); dc.LineTo(cx + 9, cy - 3);
            dc.MoveTo(cx - 9, cy + 3); dc.LineTo(cx + 9, cy + 3);
            dc.MoveTo(cx - 3, cy - 9); dc.LineTo(cx - 3, cy + 9);
            dc.MoveTo(cx + 3, cy - 9); dc.LineTo(cx + 3, cy + 9);
        }
        else if (tag == L"TreeView" || tag == L"Tree")
        {
            dc.MoveTo(cx, cy - 8); dc.LineTo(cx, cy + 8);
            dc.MoveTo(cx, cy - 4); dc.LineTo(cx + 6, cy - 4);
            dc.MoveTo(cx, cy + 2); dc.LineTo(cx + 6, cy + 2);
            dc.Ellipse(cx - 2, cy - 10, cx + 2, cy - 6);
        }
        dc.SelectObject(oldBrush);
        dc.SelectObject(oldPen);
    }

    inline CString TagFromToolName(const CString& name)
    {
        if (name == L"toolButton") return L"Button";
        if (name == L"toolTextBlock") return L"TextBlock";
        if (name == L"toolTextBox") return L"TextBox";
        if (name == L"toolCheckBox") return L"CheckBox";
        if (name == L"toolComboBox") return L"ComboBox";
        if (name == L"toolListBox") return L"ListBox";
        if (name == L"toolRadioButton") return L"RadioButton";
        if (name == L"toolPictureBox") return L"Image";
        if (name == L"toolLabel") return L"Label";
        if (name == L"toolProgressBar") return L"ProgressBar";
        if (name == L"toolListView") return L"ListView";
        if (name == L"toolTreeView") return L"TreeView";
        if (name == L"toolTabControl") return L"TabControl";
        if (name == L"toolBorder") return L"Border";
        if (name == L"toolGroupBox") return L"GroupBox";
        if (name == L"toolPanel") return L"Panel";
        if (name == L"toolSplitContainer") return L"Splitter";
        if (name == L"toolMenuStrip") return L"MenuBar";
        if (name == L"toolToolStrip") return L"ToolBar";
        if (name == L"toolStatusStrip") return L"StatusBar";
        if (name == L"toolTimer") return L"Timer";
        if (name == L"toolPrintDocument") return L"PrintDocument";
        if (name == L"toolOpenFileDialog") return L"OpenFileDialog";
        if (name == L"toolSaveFileDialog") return L"SaveFileDialog";
        if (name == L"toolDataGridView") return L"DataGridView";
        return L"";
    }
}
