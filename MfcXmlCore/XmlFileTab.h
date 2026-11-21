// XmlFileTab.h —— 多文档文件页签条（CAD 多图纸切换），三端共用。
//
// 与 XmlRibbon.h 同一套路：模型 / 解析 / 布局 / 绘制 / 命中全部在这个头里，
// 只依赖 XmlCanvas 抽象接口 + XmlElement（无 MFC），设计器画布、预览器、
// CAD 运行时共用同一份绘制与命中。状态（hot/pressed/scrollX）由宿主持有。
//
// XML：
//   <XFileTab ShowAdd="true" ActiveIndex="0">
//     <XFileTabItem Text="平面图.dwg" Active="true"/>
//     <XFileTabItem Text="总装.dwg" Dirty="true" Closable="true"/>
//   </XFileTab>
#pragma once

#include <algorithm>

#include "XmlControls.h"
#include "XmlCore.h"
#include "controls/XmlMfcTags.h"

namespace xmlctl {

// ---------------------------------------------------------------- 数据模型
struct FileTabItem
{
    std::wstring text;          // 页签显示文件名
    bool dirty    = false;     // 未保存（圆点标记）
    bool closable = true;       // 是否带关闭 ×
};

struct FileTabModel
{
    std::vector<FileTabItem> items;
    int  activeIndex = 0;       // 当前激活页签下标
    bool showAdd     = true;    // 末尾是否显示 + 新建页签
};

// ---------------------------------------------------------------- 尺寸
struct FileTabMetrics
{
    int padX    = 10;          // 页签左右内边距
    int gap     = 2;           // 相邻页签间距
    int closeW  = 18;          // 关闭框宽
    int addW    = 28;          // 末尾 + 按钮宽
    int minW    = 70;          // 页签最小宽
    int pt      = 9;           // 文字字号
};

// ---------------------------------------------------------------- 解析
inline bool filetabBool(const std::wstring& v, bool def)
{
    if (v.empty()) return def;
    return _wcsicmp(v.c_str(), L"0")   != 0 &&
           _wcsicmp(v.c_str(), L"false") != 0 &&
           _wcsicmp(v.c_str(), L"no")   != 0;
}

inline bool filetabTagIs(const std::wstring& tag, const wchar_t* a, const wchar_t* b)
{
    const std::wstring bare = xmlmfc::BareTag(tag);
    return _wcsicmp(bare.c_str(), a) == 0 || (b != nullptr && _wcsicmp(bare.c_str(), b) == 0);
}

inline void filetabParse(XmlElement& node, FileTabModel& m)
{
    // 结构每帧重建；当前激活页由宿主状态保留，不被 XML 覆盖（XML 显式指定时才改）
    const int keepActive = m.activeIndex;
    m.items.clear();

    int active = -1;
    for (auto& it : node.children)
    {
        if (!filetabTagIs(it.tag, L"FileTabItem", L"FileTab")) continue;
        FileTabItem fi;
        fi.text = it.get(L"Text");
        if (fi.text.empty()) fi.text = it.get(L"Title");
        fi.dirty    = filetabBool(it.get(L"Dirty"), false);
        fi.closable = filetabBool(it.get(L"Closable"), true);
        if (filetabBool(it.get(L"Active"), false)) active = (int)m.items.size();
        m.items.push_back(fi);
    }

    const std::wstring sa = node.get(L"ShowAdd");
    if (!sa.empty()) m.showAdd = filetabBool(sa, true);

    const std::wstring ai = node.get(L"ActiveIndex");
    if (!ai.empty()) active = _wtoi(ai.c_str());
    else if (active < 0) active = keepActive;

    if (active < 0 || active >= (int)m.items.size()) active = 0;
    m.activeIndex = active;
}

// ---------------------------------------------------------------- 布局结果
struct FileTabLayout
{
    std::vector<RECT> tabs;          // 每个页签体矩形
    std::vector<RECT> closeBoxes;     // 每个页签的关闭框（不可关闭时为空矩形）
    RECT addRc = { 0, 0, 0, 0 };     // 末尾 + 按钮
    int totalWidth  = 0;              // 内容自然总宽（含间距与 +）
    int scrollX     = 0;             // 当前横向滚动偏移
};

// 文字量宽：后端无测量能力时退回按字符宽估算（中文整字宽，ASCII 半宽）
inline int filetabTextWidth(XmlCanvas& cv, const std::wstring& s, int pt)
{
    const float w = cv.textWidth(s, pt, std::wstring());
    if (w >= 0.0f) return (int)w;
    int px = 0;
    for (const wchar_t c : s)
        px += (c >= 0x80) ? (pt * 4 / 3) : (pt * 2 / 3);
    return px;
}

// 页签宽度按文字自适应；相邻矩形不重叠；总宽超出可用宽时按 scrollX 横向裁切。
inline void filetabLayout(const FileTabModel& m, const FileTabMetrics& mx,
                          const RECT& rc, XmlCanvas& cv,
                          FileTabLayout& out, int scrollX)
{
    out.tabs.clear();
    out.closeBoxes.clear();
    out.addRc = { 0, 0, 0, 0 };

    const int avail = rc.right - rc.left;

    std::vector<int> widths(m.items.size());
    int total = 0;
    for (size_t i = 0; i < m.items.size(); ++i)
    {
        const int tw = filetabTextWidth(cv, m.items[i].text, mx.pt);
        int w = mx.padX * 2 + tw + (m.items[i].closable ? mx.closeW : 0);
        w = (std::max)(w, mx.minW);
        widths[i] = w;
        total += w + (i + 1 < m.items.size() ? mx.gap : 0);
    }
    if (m.showAdd) total += mx.addW + mx.gap;
    out.totalWidth = total;

    const int maxScroll = (std::max)(0, total - avail);
    if (scrollX < 0) scrollX = 0;
    if (scrollX > maxScroll) scrollX = maxScroll;
    out.scrollX = scrollX;

    int x = rc.left - scrollX;
    for (size_t i = 0; i < m.items.size(); ++i)
    {
        out.tabs.push_back(RECT{ x, rc.top, x + widths[i], rc.bottom });
        RECT cb = { 0, 0, 0, 0 };
        if (m.items[i].closable)
            cb = RECT{ out.tabs.back().right - mx.closeW, rc.top,
                       out.tabs.back().right, rc.bottom };
        out.closeBoxes.push_back(cb);
        x += widths[i] + mx.gap;
    }
    if (m.showAdd)
        out.addRc = RECT{ x, rc.top, x + mx.addW, rc.bottom };
}

// ---------------------------------------------------------------- 绘制
inline void filetabDraw(XmlCanvas& cv, const RECT& rc, const FileTabModel& m,
                        const FileTabMetrics& mx, const FileTabLayout& lay,
                        int hotTab, int hotClose, bool hotAdd, bool pressed, int pt)
{
    const ShellTheme& th = shell();

    cv.fillRect(rc, th.header);
    cv.fillRect(RECT{ rc.left, rc.bottom - 1, rc.right, rc.bottom }, th.line);

    cv.clipPush(rc);

    for (size_t i = 0; i < lay.tabs.size() && i < m.items.size(); ++i)
    {
        const RECT& r  = lay.tabs[i];
        const FileTabItem& it = m.items[i];
        const bool active = (int)i == m.activeIndex;
        const bool hot    = (int)i == hotTab;

        // 页签面：活跃明显抬亮，非活跃弱化，悬停给一档过渡
        if (active)      cv.fillRect(r, th.panelAlt);
        else if (hot)    cv.fillRect(r, th.panel);
        // 活跃页签底部强调下划线
        if (active)
            cv.fillRect(RECT{ r.left + 6, r.bottom - 3, r.right - 6, r.bottom - 1 }, th.accent);

        int tx = r.left + mx.padX;
        // 脏标记：文件名左侧圆点
        if (it.dirty)
        {
            const int cy = (r.top + r.bottom) / 2;
            cv.fillEllipse(RECT{ tx, cy - 3, tx + 6, cy + 3 }, th.good, th.good);
            tx += 12;
        }

        // 文字区：右边给关闭框让位；宽度按文字量过，不会裁出半个字
        RECT tr = { tx, r.top, r.right - (it.closable ? mx.closeW : mx.padX), r.bottom };
        cv.setTextStyle(active ? L"SemiBold" : L"Normal", L"Normal", L"NoWrap", L"VCenter");
        cv.text(tr, it.text, active ? RGB(255, 255, 255) : (hot ? th.text : th.textDim),
                DT_LEFT | DT_VCENTER, pt, std::wstring());
        cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");

        // 关闭 ×
        if (it.closable && i < lay.closeBoxes.size())
        {
            const RECT& cb = lay.closeBoxes[i];
            const bool chot = (int)i == hotClose;
            if (chot)
                cv.fillRoundRect(RECT{ cb.left + 2, cb.top + 4, cb.right - 2, cb.bottom - 4 },
                                 th.panelAlt, th.panelAlt, 3);
            const COLORREF c = chot ? th.text : th.textDim;
            const int cx = (cb.left + cb.right) / 2;
            const int cy = (cb.top + cb.bottom) / 2;
            const int g = 4;
            cv.strokeLine({ cx - g, cy - g }, { cx + g, cy + g }, c, 1);
            cv.strokeLine({ cx + g, cy - g }, { cx - g, cy + g }, c, 1);
        }
    }

    // 末尾 + 新建
    if (lay.addRc.right > lay.addRc.left)
    {
        const RECT& a = lay.addRc;
        if (hotAdd)
            cv.fillRoundRect(RECT{ a.left + 3, a.top + 4, a.right - 3, a.bottom - 4 },
                             th.panelAlt, th.panelAlt, 3);
        const COLORREF c = hotAdd ? th.text : th.textDim;
        const int cx = (a.left + a.right) / 2;
        const int cy = (a.top + a.bottom) / 2;
        const int g = 6;
        cv.strokeLine({ cx - g, cy }, { cx + g, cy }, c, 1);
        cv.strokeLine({ cx, cy - g }, { cx, cy + g }, c, 1);
    }

    cv.clipPop();
    (void)pressed;
}

// ---------------------------------------------------------------- 命中
struct FileTabHit
{
    int  tab    = -1;      // 命中页签下标（-1 = 未命中页签）
    int  region  = 0;       // 0 空白 / 1 页签体 / 2 关闭框 / 3 + 号
};

inline FileTabHit filetabHitTest(const FileTabLayout& lay, POINT pt)
{
    FileTabHit hit;
    if (lay.addRc.right > lay.addRc.left && PtInRect(&lay.addRc, pt))
    {
        hit.region = 3;
        return hit;
    }
    for (size_t i = 0; i < lay.tabs.size(); ++i)
    {
        if (!PtInRect(&lay.tabs[i], pt)) continue;
        hit.tab = (int)i;
        if (i < lay.closeBoxes.size() && lay.closeBoxes[i].right > lay.closeBoxes[i].left &&
            PtInRect(&lay.closeBoxes[i], pt))
            hit.region = 2;
        else
            hit.region = 1;
        return hit;
    }
    return hit;
}

}  // namespace xmlctl
