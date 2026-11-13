import io,re
h='''#pragma once
#include <windows.h>
#include <commctrl.h>
#include <vector>

// =====================================================================
// Table —— 表格控件（SysListView32 的唯一几何真相来源）
// 背景：之前"点击某行→蓝框落到下一行""原位编辑框与选中行错开一行"，
// 根因是命中行号、选中行号、编辑框矩形分别由三处各算一套。
// 这个类把 rows / cols / rect / cell 收进一个对象，调用方只问它：
//   rect()      -> 控件客户区
//   cell(r,c)   -> 第 r 行第 c 列格的矩形（客户坐标）
//   rowAt(y)    -> 客户 y 落在第几行（-1 = 表头或空白）
//   cellAt(x,y) -> 命中 (row,col)，与 cell(row,col) 完全一致
// 命中、选中、编辑框从此共用同一份计算，不会再差一行。
// =====================================================================
class Table
{
public:
    Table() : m_hwnd(nullptr) {}
    explicit Table(HWND hwnd) : m_hwnd(hwnd) {}

    void attach(HWND hwnd) { m_hwnd = hwnd; }
    HWND hwnd() const { return m_hwnd; }
    bool valid() const { return m_hwnd != nullptr && ::IsWindow(m_hwnd) == TRUE; }

    int rows() const;                        // 行数
    int cols() const;                        // 列数
    std::vector<int> colWidths() const;      // 每列宽度
    int headerHeight() const;                // 表头高度（= 第 0 行矩形 top）
    int rowHeight() const;                   // 行高（第 0 行矩形高度）

    RECT rect() const;                       // 控件客户区
    RECT rowRect(int row) const;             // 第 row 行矩形（失败返回全 0）
    RECT cell(int row, int col) const;       // 单元格矩形（表头/空白/越界返回全 0）

    int rowAt(int y) const;                  // 客户 y → 行号，-1 表示不在数据行上
    bool cellAt(int x, int y, int& row, int& col) const;  // 命中，与 cell() 同源

    int selectedRow() const;                 // 原生选中行，-1 无
    void selectRow(int row);                 // 设选中行（clamp 到 [0, rows-1]）
    void ensureVisible(int row);
private:
    HWND m_hwnd;
};
'''
c='''#include "stdafx.h"
#include "Table.h"

namespace
{
    bool isEmpty(const RECT& r)
    {
        return r.right <= r.left || r.bottom <= r.top;
    }
}

int Table::rows() const
{
    if (!valid()) return 0;
    return static_cast<int>(::SendMessageW(m_hwnd, LVM_GETITEMCOUNT, 0, 0));
}

int Table::cols() const
{
    if (!valid()) return 0;
    HWND hh = reinterpret_cast<HWND>(::SendMessageW(m_hwnd, LVM_GETHEADER, 0, 0));
    if (hh == nullptr) return 0;
    return static_cast<int>(::SendMessageW(hh, HDM_GETITEMCOUNT, 0, 0));
}

std::vector<int> Table::colWidths() const
{
    std::vector<int> ws;
    const int n = cols();
    ws.reserve(n);
    for (int i = 0; i < n; ++i)
        ws.push_back(static_cast<int>(::SendMessageW(m_hwnd, LVM_GETCOLUMNWIDTH,
                                                     static_cast<WPARAM>(i), 0)));
    return ws;
}

RECT Table::rect() const
{
    RECT rc = { 0, 0, 0, 0 };
    if (valid()) ::GetClientRect(m_hwnd, &rc);
    return rc;
}

RECT Table::rowRect(int row) const
{
    RECT rc = { 0, 0, 0, 0 };
    if (!valid() || row < 0 || row >= rows()) return rc;
    if (::SendMessageW(m_hwnd, LVM_GETITEMRECT, static_cast<WPARAM>(row),
                       reinterpret_cast<LPARAM>(&rc)) == FALSE)
    {
        RECT empty = { 0, 0, 0, 0 };
        return empty;
    }
    return rc;
}

int Table::headerHeight() const
{
    const RECT r0 = rowRect(0);
    return isEmpty(r0) ? 0 : r0.top;
}

int Table::rowHeight() const
{
    const RECT r0 = rowRect(0);
    return isEmpty(r0) ? 0 : (r0.bottom - r0.top);
}

RECT Table::cell(int row, int col) const
{
    RECT rc = { 0, 0, 0, 0 };
    if (!valid() || row < 0 || col < 0 || col >= cols()) return rc;
    const RECT rr = rowRect(row);
    if (isEmpty(rr)) return rc;

    // LVM_GETSUBITEMRECT 在本机 SysListView32 上水平方向恒返回 0 宽，
    // 所以按"行矩形 + 累加列宽"算左右边界；纵向直接用行矩形。
    int x = 0, w = 0;
    for (int i = 0; i <= col; ++i)
    {
        const int cw = static_cast<int>(::SendMessageW(m_hwnd, LVM_GETCOLUMNWIDTH,
                                                       static_cast<WPARAM>(i), 0));
        if (cw <= 0) { RECT empty = { 0, 0, 0, 0 }; return empty; }
        if (i < col) x += cw; else w = cw;
    }
    POINT org = { 0, 0 };
    ::SendMessageW(m_hwnd, LVM_GETORIGIN, 0, reinterpret_cast<LPARAM>(&org));
    rc.left = x - org.x;
    rc.right = rc.left + w;
    rc.top = rr.top - org.y;
    rc.bottom = rr.bottom - org.y;
    return rc;
}

int Table::rowAt(int y) const
{
    if (!valid()) return -1;
    const int n = rows();
    if (n <= 0) return -1;
    const int hh = headerHeight();
    const int rh = rowHeight();
    if (rh <= 0) return -1;
    POINT org = { 0, 0 };
    ::SendMessageW(m_hwnd, LVM_GETORIGIN, 0, reinterpret_cast<LPARAM>(&org));
    const int yy = y + org.y;
    if (yy < hh) return -1;
    const int idx = (yy - hh) / rh;
    if (idx < 0 || idx >= n) return -1;
    return idx;
}

bool Table::cellAt(int x, int y, int& row, int& col) const
{
    row = -1;
    col = -1;
    if (!valid()) return false;
    LVHITTESTINFO hi = {};
    hi.pt.x = x;
    hi.pt.y = y;
    ::SendMessageW(m_hwnd, LVM_SUBITEMHITTEST, 0, reinterpret_cast<LPARAM>(&hi));
    if (hi.iItem < 0 || hi.iItem >= rows())
    {
        // 交回给 rowAt 兜底（表头/空白行被 hittest 吃掉时也不会差一行）
        const int r = rowAt(y);
        if (r < 0) return false;
        row = r;
        const std::vector<int> ws = colWidths();
        int acc = 0;
        for (int i = 0; i < static_cast<int>(ws.size()); ++i)
        {
            if (x >= acc && x < acc + ws[i]) { col = i; break; }
            acc += ws[i];
        }
        if (col < 0) col = 0;
        return true;
    }
    row = hi.iItem;
    col = hi.iSubItem >= 0 ? hi.iSubItem : 0;
    return true;
}

int Table::selectedRow() const
{
    if (!valid()) return -1;
    const int r = static_cast<int>(::SendMessageW(m_hwnd, LVM_GETNEXTITEM,
                                                  static_cast<WPARAM>(-1), LVNI_SELECTED));
    return r;
}

void Table::selectRow(int row)
{
    if (!valid()) return;
    const int n = rows();
    if (n <= 0) return;
    int v = row < 0 ? -1 : (row >= n ? n - 1 : row);
    if (v < 0) return;
    LVITEMW lv = {};
    lv.mask = LVIF_STATE;
    lv.state = LVIS_SELECTED | LVIS_FOCUSED;
    lv.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    ::SendMessageW(m_hwnd, LVM_SETITEMSTATE, static_cast<WPARAM>(v),
                   reinterpret_cast<LPARAM>(&lv));
}

void Table::ensureVisible(int row)
{
    if (!valid()) return;
    ::SendMessageW(m_hwnd, LVM_ENSUREVISIBLE, static_cast<WPARAM>(row), FALSE);
}
'''
io.open('G:/PyArx/PyMfcXaml/Table.h','w',encoding='utf-8-sig',newline='').write(h.replace('\n','\r\n'))
io.open('G:/PyArx/PyMfcXaml/Table.cpp','w',encoding='utf-8-sig',newline='').write(c.replace('\n','\r\n'))
p='G:/PyArx/PyMfcXaml/PyMfcXaml.vcxproj'
s=io.open(p,encoding='utf-8-sig').read()
if 'Table.cpp' not in s:
    m=re.search(r'(\s*)<ClCompile Include="[^"]+"\s*/>', s)
    s=s[:m.end()]+m.group(1)+'<ClCompile Include="Table.cpp" />'+s[m.end():]
io.open(p,'w',encoding='utf-8-sig',newline='').write(s)
print('Table.h/.cpp 已建，vcxproj 已加 Table.cpp')
