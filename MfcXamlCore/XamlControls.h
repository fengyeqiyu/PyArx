// XamlControls.h —— 三端共用的"复合控件绘制"层
//
// 为什么单独一层：TreeView / PropertyGrid / Splitter / StatusBar 这些不是单个
// 矩形+文字，而是"容器 + 多行 + 选中态 + 分隔线"的组合绘制。设计器画布、
// 预览器、CAD 运行时都要画它们，且必须长得一样，所以逻辑只能有一份。
//
// 只依赖 XamlCanvas 抽象接口（fillRect / text / strokeRect / ...），
// 因此 GDI 后端和 D2D 后端都能用，天然满足"三端一致"。
#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "XamlDraw.h"

namespace xamlctl {

// ---------------------------------------------------------------- 主题
// 设计器 / 预览器 / 运行时的深色面板配色。三端取同一组值，
// 否则"设计器里灰一点、CAD 里蓝一点"这种漂移会反复出现。
struct Theme
{
    COLORREF panel      = RGB(62, 78, 90);    // #3E4E5A 面板标题 / 状态栏
    COLORREF panelBody  = RGB(43, 52, 62);    // 深色主体（树、属性表底）
    COLORREF codeBody   = RGB(30, 30, 30);    // #1E1E1E 代码区
    COLORREF row        = RGB(43, 52, 62);
    COLORREF rowAlt     = RGB(38, 46, 55);
    COLORREF keyCol     = RGB(52, 64, 75);
    COLORREF text       = RGB(220, 220, 220);
    COLORREF textDim    = RGB(150, 158, 170);
    COLORREF accent     = RGB(45, 107, 214);
    COLORREF accentHot  = RGB(70, 130, 200);
    COLORREF guide      = RGB(96, 110, 122);
    COLORREF line       = RGB(64, 78, 90);
    COLORREF white      = RGB(255, 255, 255);
};

inline const Theme& theme()
{
    static Theme t;
    return t;
}

// ---------------------------------------------------------------- 文档级列表控件
// ListBox / ListView / TreeView / PropertyGrid 在"文档"里通常只是一个空壳。
// 未声明外观 = 系统窗口外观（白底 + 凹陷边框 + 逐行条目），声明了外观就走
// xamlDrawBackplate。三端都调这一个函数，避免"预览器浅色、设计器深色"的漂移。
inline void xamlDrawListLike(XamlCanvas& cv, const RECT& rc, const XamlElement& el,
                             bool styled, const xamlapp::XamlAppearance& ap,
                             COLORREF ink, COLORREF edge, int lineH = 18)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;

    const COLORREF win = ::GetSysColor(COLOR_WINDOW);
    if (styled)
        xamlDrawBackplate(cv, rc, ap, win, edge);
    else
    {
        cv.fillRect(rc, win);
        cv.frame3d(rc, edge, true, 2);
    }

    cv.clipPush(rc);
    int y = (int)rc.top + 3;
    if (!el.children.empty())
    {
        for (const auto& c : el.children)
        {
            if (y + lineH > (int)rc.bottom) break;
            RECT tr = { rc.left + 4, y, rc.right - 2, y + lineH };
            const std::wstring s = c.text.empty() ? c.tag : c.text;
            xamlDrawText(cv, tr, s, ap, false, ink, DT_LEFT, 0);
            y += lineH;
        }
    }
    else if (!el.text.empty())
    {
        RECT tr = { rc.left + 4, rc.top + 2, rc.right - 2, rc.bottom - 2 };
        xamlDrawText(cv, tr, el.text, ap, false, ink, DT_LEFT, 0);
    }
    cv.clipPop();
}

// ---------------------------------------------------------------- 树
struct TreeRow
{
    std::wstring text;
    int  level       = 0;      // 缩进层级（0 = 一级）
    bool hasChildren = false;  // 画 +/- 展开框
    bool expanded    = false;
    bool selected    = false;
};

struct TreeStyle
{
    // 默认 = 系统窗口外观（XAML 未声明外观时就该长这样，与 CTreeCtrl 对齐）
    COLORREF back    = RGB(255, 255, 255);
    COLORREF fore    = RGB(30, 30, 30);
    COLORREF selBack = RGB(0, 120, 215);
    COLORREF selFore = RGB(255, 255, 255);
    COLORREF guide   = RGB(118, 118, 118);
    int rowH     = 24;
    int indent   = 16;
    int padLeft  = 8;
};

// 返回内容总高度（供调用方算滚动范围 / 命中测试）
inline int xamlDrawTree(XamlCanvas& cv, const RECT& rc, const std::vector<TreeRow>& rows,
                        const TreeStyle& st = TreeStyle(), int scrollY = 0,
                        int ptSize = 0, const std::wstring& face = std::wstring())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return 0;
    cv.fillRect(rc, st.back);

    cv.clipPush(rc);
    int y = rc.top - scrollY;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const TreeRow& r = rows[i];
        RECT rowRc = { rc.left, y, rc.right - 1, y + st.rowH };
        y += st.rowH;
        if (rowRc.bottom <= rc.top) continue;
        if (rowRc.top >= rc.bottom) break;

        if (r.selected)
            cv.fillRect(rowRc, st.selBack);

        int x = rc.left + st.padLeft + r.level * st.indent;
        if (r.hasChildren)
        {
            RECT box = { x, rowRc.top + (st.rowH - 9) / 2, x + 9, rowRc.top + (st.rowH - 9) / 2 + 9 };
            cv.strokeRect(box, r.selected ? st.selFore : st.guide, 1);
            cv.fillRect(RECT{ box.left + 2, box.top + 4, box.right - 2, box.top + 5 },
                        r.selected ? st.selFore : st.guide);
            if (!r.expanded)
                cv.fillRect(RECT{ box.left + 4, box.top + 2, box.left + 5, box.bottom - 2 },
                            r.selected ? st.selFore : st.guide);
        }
        x += 13;

        RECT tr = { x, rowRc.top, rc.right - 4, rowRc.bottom };
        cv.text(tr, r.text, r.selected ? st.selFore : st.fore, DT_LEFT, ptSize, face);
    }
    cv.clipPop();

    return (int)rows.size() * st.rowH;
}

// ---------------------------------------------------------------- 属性表
struct PropRow
{
    std::wstring key;
    std::wstring val;
    bool selected = false;
    bool editable = false;     // 值列画成"可编辑"样式（下沉框）
    bool group = false;        // 分组标题行（"布局"/"属性"），不可编辑
};

struct PropStyle
{
    // 默认 = 系统报表外观（与 CListCtrl LVS_REPORT 对齐）
    COLORREF back      = RGB(255, 255, 255);
    COLORREF keyBack   = RGB(245, 245, 245);
    COLORREF keyFore   = RGB(30, 30, 30);
    COLORREF valFore   = RGB(30, 30, 30);
    COLORREF line      = RGB(216, 216, 216);
    COLORREF selBack   = RGB(0, 120, 215);
    COLORREF editBack  = RGB(255, 255, 255);
    COLORREF groupBack = RGB(232, 232, 232);   // 分组行（"布局"/"属性"）
    int rowH    = 24;
    int keyW    = 96;
    int padLeft = 8;
    bool head   = true;                        // 顶部表头（"属性" | "值"）
    int headH   = 23;
};

inline int xamlDrawPropertyGrid(XamlCanvas& cv, const RECT& rc, const std::vector<PropRow>& rows,
                                const PropStyle& st = PropStyle(), int scrollY = 0,
                                int ptSize = 0, const std::wstring& face = std::wstring())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return 0;
    cv.fillRect(rc, st.back);

    // 列分隔线
    const int keyRight = (st.keyW > 0) ? (std::min)((int)rc.left + st.keyW, (int)rc.right) : (int)rc.left;
    const int contentTop = rc.top + (st.head ? st.headH : 0);

    // 表头（"属性" | "值"）
    if (st.head)
    {
        RECT hdr = { rc.left, rc.top, rc.right - 1, contentTop };
        cv.fillRect(hdr, st.keyBack);
        RECT hk = { rc.left + st.padLeft, hdr.top, keyRight - 3, hdr.bottom };
        cv.text(hk, L"属性", st.keyFore, DT_LEFT, ptSize, face);
        RECT hv = { keyRight + st.padLeft, hdr.top, rc.right - 3, hdr.bottom };
        cv.text(hv, L"值", st.keyFore, DT_LEFT, ptSize, face);
        cv.fillRect(RECT{ rc.left, contentTop - 1, rc.right - 1, contentTop }, st.line);
    }

    cv.clipPush(RECT{ rc.left, contentTop, rc.right, rc.bottom });
    int y = contentTop - scrollY;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const PropRow& r = rows[i];
        RECT rowRc = { rc.left, y, rc.right - 1, y + st.rowH };
        y += st.rowH;
        if (rowRc.bottom <= contentTop) continue;
        if (rowRc.top >= rc.bottom) break;

        // 分组行（"布局" / "属性"）：整行底色 + 单列文字
        if (r.val.empty())
        {
            cv.fillRect(rowRc, st.groupBack);
            RECT g = { rc.left + st.padLeft, rowRc.top, rc.right - 3, rowRc.bottom };
            cv.text(g, r.key, st.keyFore, DT_LEFT, ptSize, face);
            cv.fillRect(RECT{ rowRc.left, rowRc.bottom, rowRc.right, rowRc.bottom + 1 }, st.line);
            continue;
        }

        if (r.selected)
        {
            cv.fillRect(rowRc, st.selBack);
        }
        else
        {
            cv.fillRect(RECT{ rc.left, rowRc.top, keyRight, rowRc.bottom }, st.keyBack);
            cv.fillRect(RECT{ keyRight, rowRc.top, rc.right - 1, rowRc.bottom }, st.back);
        }

        RECT k = { rc.left + st.padLeft, rowRc.top, keyRight - 3, rowRc.bottom };
        cv.text(k, r.key, r.selected ? RGB(255, 255, 255) : st.keyFore, DT_LEFT, ptSize, face);

        RECT v = { keyRight + st.padLeft, rowRc.top, rc.right - 3, rowRc.bottom };
        if (!r.selected && r.editable)
        {
            RECT box = { v.left - 2, rowRc.top + 3, v.right + 2, rowRc.bottom - 3 };
            cv.fillRect(box, st.editBack);
            cv.strokeRect(box, st.line, 1);
        }
        cv.text(v, r.val, r.selected ? RGB(255, 255, 255) : st.valFore, DT_LEFT, ptSize, face);

        cv.fillRect(RECT{ rowRc.left, rowRc.bottom, rowRc.right, rowRc.bottom + 1 }, st.line);
    }
    cv.clipPop();

    // 列分隔线（表头与内容一起贯穿）
    cv.fillRect(RECT{ keyRight - 1, rc.top, keyRight, rc.bottom }, st.line);
    return (int)rows.size() * st.rowH + (st.head ? st.headH : 0);
}

// ---------------------------------------------------------------- 分隔条
inline void xamlDrawSplitter(XamlCanvas& cv, const RECT& rc, bool vertical,
                             bool hot = false, bool dragging = false,
                             COLORREF back = RGB(38, 47, 56))
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    cv.fillRect(rc, back);
    if (!hot && !dragging) return;

    const COLORREF c = dragging ? RGB(110, 175, 245) : RGB(70, 130, 200);
    if (vertical)
        cv.fillRect(RECT{ rc.left, rc.top, rc.right - 1, rc.bottom }, c);
    else
        cv.fillRect(RECT{ rc.left, rc.top, rc.right, rc.bottom - 1 }, c);

    // 中央三个抓手点
    const int cx = (rc.left + rc.right) / 2;
    const int cy = (rc.top + rc.bottom) / 2;
    const int d = 2;
    for (int i = -1; i <= 1; ++i)
    {
        if (vertical)
        {
            RECT r = { cx - d + 1, cy + i * 4 - d, cx + d, cy + i * 4 + d + 1 };
            cv.fillRect(r, RGB(230, 240, 250));
        }
        else
        {
            RECT r = { cx + i * 4 - d, cy - d + 1, cx + i * 4 + d + 1, cy + d };
            cv.fillRect(r, RGB(230, 240, 250));
        }
    }
}

// ---------------------------------------------------------------- 状态栏
struct StatusCell
{
    std::wstring text;
    int width = 0;          // 0 = 自适应剩余
};

inline void xamlDrawStatusBar(XamlCanvas& cv, const RECT& rc, const std::vector<StatusCell>& cells,
                              COLORREF back = RGB(0, 120, 215),
                              COLORREF fore = RGB(255, 255, 255),
                              int gap = 8, int ptSize = 0,
                              const std::wstring& face = std::wstring())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    cv.fillRect(rc, back);

    cv.clipPush(rc);
    int x = rc.left + gap;
    for (size_t i = 0; i < cells.size(); ++i)
    {
        const StatusCell& c = cells[i];
        int w = (c.width > 0) ? c.width : (rc.right - x - gap);
        if (x + w > rc.right - 2) w = rc.right - x - 2;
        if (w <= 0) break;
        RECT r = { x, rc.top, x + w, rc.bottom };
        cv.text(r, c.text, fore, DT_LEFT, ptSize, face);
        x += w + gap;
    }
    cv.clipPop();
}

// ---------------------------------------------------------------- 工具栏
struct ToolButtonStyle
{
    COLORREF back    = RGB(62, 78, 90);
    COLORREF hot     = RGB(82, 102, 118);
    COLORREF pressed = RGB(46, 58, 68);
    int radius = 4;
    int pad    = 4;
};

// 图标按钮：圆角块 + PNG 图标（图标走 XamlCanvas::drawImage，D2D 后端用 WIC 解码）
inline void xamlDrawToolButton(XamlCanvas& cv, const RECT& rc, const std::wstring& iconPath,
                               bool hot = false, bool pressed = false,
                               const ToolButtonStyle& st = ToolButtonStyle())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    const COLORREF b = pressed ? st.pressed : (hot ? st.hot : st.back);
    cv.fillRoundRect(rc, b, b, st.radius);
    if (!iconPath.empty())
    {
        RECT ir = rc;
        ::InflateRect(&ir, -st.pad, -st.pad);
        if (ir.right > ir.left && ir.bottom > ir.top)
            cv.drawImage(ir, iconPath);
    }
}

// 工具栏分隔线：竖条
inline void xamlDrawSeparator(XamlCanvas& cv, const RECT& rc, bool vertical = true,
                              COLORREF c = RGB(90, 106, 120))
{
    if (vertical)
    {
        const int x = (rc.left + rc.right) / 2;
        cv.fillRect(RECT{ x, rc.top + 4, x + 1, rc.bottom - 4 }, c);
    }
    else
    {
        const int y = (rc.top + rc.bottom) / 2;
        cv.fillRect(RECT{ rc.left + 4, y, rc.right - 4, y + 1 }, c);
    }
}

// ---------------------------------------------------------------- 菜单条
// 系统菜单由 OS 绘制（非客户区之外的自绘菜单条才需要这个）
inline void xamlDrawMenuBar(XamlCanvas& cv, const RECT& rc, const std::vector<std::wstring>& items,
                            COLORREF back = RGB(62, 78, 90), COLORREF fore = RGB(230, 230, 230),
                            int padX = 12, int ptSize = 0, const std::wstring& face = std::wstring())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    cv.fillRect(rc, back);
    int x = rc.left + 4;
    for (const auto& s : items)
    {
        RECT r = { x, rc.top, x + padX * 2 + (int)s.size() * 14, rc.bottom };
        cv.text({ r.left + padX, r.top, r.right, r.bottom }, s, fore, DT_LEFT, ptSize, face);
        x = r.right;
    }
}

// ---------------------------------------------------------------- 面板标题条
inline void xamlDrawPanelCaption(XamlCanvas& cv, const RECT& rc, const std::wstring& title,
                                 COLORREF back = RGB(62, 78, 90),
                                 COLORREF fore = RGB(220, 220, 220),
                                 int padLeft = 6, int ptSize = 0,
                                 const std::wstring& face = std::wstring())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    cv.fillRect(rc, back);
    RECT tr = { rc.left + padLeft, rc.top, rc.right - 2, rc.bottom };
    cv.text(tr, title, fore, DT_LEFT, ptSize, face);
}

// ---------------------------------------------------------------- 设计器外壳（深色 IDE 主题）
// 设计器自身的界面同样是 XAML 声明出来的，但构成外壳的这些"构件"不是普通控件：
// 面板标题条、文档标签、分类胶囊、画布舞台、带标题的工具按钮、富状态栏。
// 它们的几何与配色只在这里定义一份，设计器与预览器都调同一批函数，
// 否则"设计器里好看、XamlViewer 里散架"这种漂移会反复出现。
struct ShellTheme
{
    COLORREF base      = RGB(21, 24, 29);    // #15181D 整窗底
    COLORREF panel     = RGB(28, 32, 39);    // #1C2027 面板底
    COLORREF panelAlt  = RGB(45, 54, 66);    // #2D3642 选中/标签面
    COLORREF header    = RGB(38, 44, 54);    // #262C36 标题条 / 工具栏
    COLORREF stage     = RGB(15, 18, 22);    // #0F1216 画布舞台
    COLORREF code      = RGB(18, 21, 26);    // #12151A 代码区
    COLORREF line      = RGB(51, 59, 70);    // #333B46 主分隔线
    COLORREF lineSoft  = RGB(42, 48, 57);    // #2A3039 次级分隔线
    COLORREF text      = RGB(228, 233, 240); // #E4E9F0
    COLORREF textDim   = RGB(150, 161, 174);// #96A1AE
    COLORREF accent    = RGB(76, 141, 255);  // #4C8DFF
    COLORREF accentDim = RGB(44, 74, 121);   // #2C4A79
    COLORREF good      = RGB(72, 178, 118);  // #48B276
};

inline const ShellTheme& shell()
{
    static ShellTheme t;
    return t;
}

// 面板标题条：底色 + 上下细线 + 左侧强调条 + 标题 + 右对齐提示
inline void xamlDrawPanelHeader(XamlCanvas& cv, const RECT& rc, const std::wstring& title,
                                const std::wstring& hint = std::wstring(), int ptSize = 0,
                                const std::wstring& face = std::wstring())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    const ShellTheme& th = shell();
    cv.fillRect(rc, th.header);
    cv.fillRect(RECT{ rc.left, rc.top, rc.right, rc.top + 1 }, th.lineSoft);
    cv.fillRect(RECT{ rc.left, rc.bottom - 1, rc.right, rc.bottom }, th.line);
    cv.fillRect(RECT{ rc.left, rc.top + 1, rc.left + 3, rc.bottom - 1 }, th.accent);

    cv.setTextStyle(L"SemiBold", L"Normal", L"NoWrap", L"Center");
    cv.text(RECT{ rc.left + 11, rc.top, rc.right - 8, rc.bottom }, title, th.text, DT_LEFT,
            ptSize, face);
    cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
    if (!hint.empty())
        cv.text(RECT{ rc.left + 8, rc.top, rc.right - 12, rc.bottom }, hint, th.textDim, DT_RIGHT,
                ptSize > 0 ? ptSize - 1 : 0, face);
}

// 文档标签条的底（填充 + 下边线），标签本身用 xamlDrawTab 逐枚画
inline void xamlDrawTabBar(XamlCanvas& cv, const RECT& rc)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    const ShellTheme& th = shell();
    cv.fillRect(rc, th.header);
    cv.fillRect(RECT{ rc.left, rc.bottom - 1, rc.right, rc.bottom }, th.line);
}

// 单枚文档标签：选中 = 抬起的圆角块 + 顶部强调条；未选中 = 只有暗色文字
inline void xamlDrawTab(XamlCanvas& cv, const RECT& rc, const std::wstring& title,
                        bool active, bool hot = false, int ptSize = 0,
                        const std::wstring& face = std::wstring())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    const ShellTheme& th = shell();
    if (active)
    {
        cv.fillRoundRect(rc, th.panelAlt, RGB(58, 68, 82), 5);
        cv.fillRect(RECT{ rc.left + 6, rc.top + 1, rc.right - 6, rc.top + 3 }, th.accent);
    }
    else if (hot)
    {
        cv.fillRoundRect(rc, RGB(40, 47, 57), RGB(40, 47, 57), 5);
    }
    cv.setTextStyle(active ? L"SemiBold" : L"Normal", L"Normal", L"NoWrap", L"Center");
    RECT tr = rc;
    tr.left += 10;
    tr.right -= 8;
    cv.text(tr, title, active ? th.text : th.textDim, DT_LEFT, ptSize, face);
    cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
}

// 分类过滤胶囊：选中 = 强调色实心；未选中 = 深色药丸 + 细边
inline void xamlDrawChip(XamlCanvas& cv, const RECT& rc, const std::wstring& text,
                         bool checked, bool hot = false, int ptSize = 0,
                         const std::wstring& face = std::wstring())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    const ShellTheme& th = shell();
    const int radius = (rc.bottom - rc.top) / 2;
    COLORREF faceC = checked ? th.accent : (hot ? RGB(45, 54, 66) : RGB(30, 35, 43));
    COLORREF edge = checked ? th.accent : th.line;
    cv.fillRoundRect(rc, faceC, edge, radius);
    cv.setTextStyle(checked ? L"SemiBold" : L"Normal", L"Normal", L"NoWrap", L"Center");
    cv.text(rc, text, checked ? RGB(255, 255, 255) : (hot ? th.text : th.textDim), DT_CENTER,
            ptSize, face);
    cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
}

// 画布舞台：深色底 + 四角定位角标，让画布像"贴在台面上的一张纸"
inline void xamlDrawStage(XamlCanvas& cv, const RECT& rc)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    const ShellTheme& th = shell();
    cv.fillRect(rc, th.stage);
    cv.strokeRect(rc, th.lineSoft, 1);

    const int len = 12, inset = 5;
    COLORREF mark = RGB(52, 62, 76);
    // 左上
    cv.fillRect(RECT{ rc.left + inset, rc.top + inset, rc.left + inset + len, rc.top + inset + 1 }, mark);
    cv.fillRect(RECT{ rc.left + inset, rc.top + inset, rc.left + inset + 1, rc.top + inset + len }, mark);
    // 右上
    cv.fillRect(RECT{ rc.right - inset - len, rc.top + inset, rc.right - inset, rc.top + inset + 1 }, mark);
    cv.fillRect(RECT{ rc.right - inset - 1, rc.top + inset, rc.right - inset, rc.top + inset + len }, mark);
    // 左下
    cv.fillRect(RECT{ rc.left + inset, rc.bottom - inset - 1, rc.left + inset + len, rc.bottom - inset }, mark);
    cv.fillRect(RECT{ rc.left + inset, rc.bottom - inset - len, rc.left + inset + 1, rc.bottom - inset }, mark);
    // 右下
    cv.fillRect(RECT{ rc.right - inset - len, rc.bottom - inset - 1, rc.right - inset, rc.bottom - inset }, mark);
    cv.fillRect(RECT{ rc.right - inset - 1, rc.bottom - inset - len, rc.right - inset, rc.bottom - inset }, mark);
}

// 工具栏按钮（图标 + 下方标题）：设计器工具栏 44px 高的那批按钮走这个
inline void xamlDrawToolButtonCaption(XamlCanvas& cv, const RECT& rc, const std::wstring& iconPath,
                                      const std::wstring& caption, bool hot = false,
                                      bool pressed = false, bool active = false,
                                      const ToolButtonStyle& st = ToolButtonStyle(), int ptSize = 0,
                                      const std::wstring& face = std::wstring())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    const ShellTheme& th = shell();
    const COLORREF b = active ? th.accentDim
                              : (pressed ? RGB(32, 39, 49) : (hot ? th.panelAlt : th.header));
    cv.fillRoundRect(rc, b, b, 5);

    const int side = 18;
    const int cx = (rc.left + rc.right) / 2;
    RECT ir = { cx - side / 2, rc.top + 7, cx - side / 2 + side, rc.top + 7 + side };
    if (ir.bottom > rc.bottom - 12) ir.bottom = rc.bottom - 12;
    if (ir.right > ir.left && ir.bottom > ir.top && !iconPath.empty())
        cv.drawImage(ir, iconPath);

    if (!caption.empty())
    {
        cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
        RECT tr = { rc.left + 2, rc.top + 26, rc.right - 2, rc.bottom - 2 };
        cv.text(tr, caption, active ? RGB(255, 255, 255) : (hot ? th.text : th.textDim),
                DT_CENTER, ptSize > 0 ? ptSize : 8, face);
    }
}

// 胶囊按钮（工具栏右端的"预览 / 校验"）：primary = 实心强调色，否则描边幽灵按钮
inline void xamlDrawPillButton(XamlCanvas& cv, const RECT& rc, const std::wstring& text,
                               bool primary, bool hot = false, int ptSize = 0,
                               const std::wstring& face = std::wstring())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    const ShellTheme& th = shell();
    const int radius = (rc.bottom - rc.top) / 2;
    COLORREF faceC = primary ? (hot ? RGB(96, 158, 255) : th.accent)
                             : (hot ? RGB(45, 54, 66) : RGB(30, 35, 43));
    COLORREF edge = primary ? faceC : (hot ? RGB(76, 141, 255) : th.line);
    cv.fillRoundRect(rc, faceC, edge, radius);
    cv.setTextStyle(primary ? L"SemiBold" : L"Normal", L"Normal", L"NoWrap", L"Center");
    cv.text(rc, text, primary ? RGB(255, 255, 255) : (hot ? th.text : th.textDim), DT_CENTER,
            ptSize, face);
    cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
}

// ---------------------------------------------------------------- 富状态栏
struct StatusCell2
{
    std::wstring text;
    int width = 0;            // 0 = 吃掉剩余宽度
    bool accent = false;      // 品牌/强调格（实心强调色块）
    UINT align = DT_LEFT;
};

inline void xamlDrawStatusBarRich(XamlCanvas& cv, const RECT& rc, const std::vector<StatusCell2>& cells,
                                  int ptSize = 0, const std::wstring& face = std::wstring())
{
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    const ShellTheme& th = shell();
    cv.fillRect(rc, RGB(23, 27, 33));
    cv.fillRect(RECT{ rc.left, rc.top, rc.right, rc.top + 1 }, th.lineSoft);

    int x = rc.left;
    for (size_t i = 0; i < cells.size(); ++i)
    {
        const StatusCell2& c = cells[i];
        int w = (c.width > 0) ? c.width : (rc.right - x);
        if (x + w > rc.right) w = rc.right - x;
        if (w <= 0) break;

        RECT r = { x, rc.top + 1, x + w, rc.bottom };
        if (c.accent)
        {
            cv.fillRect(r, th.accent);
            cv.text(RECT{ r.left + 12, r.top, r.right - 6, r.bottom }, c.text, RGB(255, 255, 255),
                    DT_LEFT, ptSize, face);
        }
        else
        {
            cv.text(RECT{ r.left + 10, r.top, r.right - 6, r.bottom }, c.text,
                    c.text == L"" ? th.textDim : th.textDim, c.align, ptSize, face);
        }
        x += w;
        if (!c.accent && i + 1 < cells.size())
            cv.fillRect(RECT{ x - 1, rc.top + 5, x, rc.bottom - 5 }, th.lineSoft);
    }
}

} // namespace xamlctl
