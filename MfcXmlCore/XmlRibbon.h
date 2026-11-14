// XmlRibbon.h —— Office 风格 Ribbon 控件（三端共用）
//
// 为什么下沉到控件库：Ribbon 不是"一堆绝对坐标按钮"，它有固定结构
// （选项卡 → 分组 → 按钮 + 组名）和固定行为（互斥选中、悬停、点击、宽度不足折叠）。
// 这些逻辑放设计器里，预览器和运行时就得再抄一遍，三端必然漂移。
//
// 这里只依赖 XmlCanvas 抽象接口 + XmlElement（无 MFC），所以设计器画布、
// 预览器、CAD 运行时共用同一份绘制与命中。
//
// XML：
//   <XRibbon Name="mainRibbon" SelectedTab="1">
//     <XRibbonTab Header="文件">
//       <XRibbonGroup Header="文件">
//         <XRibbonButton Name="btnNew" Icon="res/new.png" Text="新建"/>
//       </XRibbonGroup>
//     </XRibbonTab>
//   </XRibbon>
#pragma once

#include <algorithm>

#include "XmlControls.h"
#include "XmlCore.h"

namespace xmlctl {

// ---------------------------------------------------------------- 数据模型
struct XRibbonButton
{
    struct Item
    {
        std::wstring name, text, icon;
    };
    std::wstring name;          // 动作名（与设计器命令同名，如 btnPaste）
    std::wstring text;          // 按钮标题
    std::wstring icon;          // PNG 路径（相对 exe）
    int width  = 58;
    bool tiny = false;         // 小按钮：图标在左、文字在右，纵向三层排
    std::vector<Item> items;   // 下拉项（空 = 普通按钮）
};

// 按钮的下拉区（右下角小三角所在的位置）
inline RECT ribbonDropdownRect(const RECT& btnRc)
{
    return RECT{ btnRc.right - 15, btnRc.bottom - 13, btnRc.right - 3, btnRc.bottom - 1 };
}

struct XRibbonGroup
{
    std::wstring header;        // 组名（底部小字）
    std::wstring dialog;        // 组对话启动器（右下角斜箭头），空 = 没有
    std::vector<XRibbonButton> buttons;
};

struct XRibbonTab
{
    std::wstring header;
    std::vector<XRibbonGroup> groups;
};

// 文件页（Backstage）的左侧导航项
struct XRibbonBackstageItem
{
    std::wstring name;          // 命令名（btnNew / btnOpen / btnSave / btnExit …）
    std::wstring header;        // 左侧导航文字
    std::wstring text;          // 右侧说明（可选）
    std::wstring icon;
    RECT rc = { 0, 0, 0, 0 };   // 由 ribbonBackstageLayout 填
};

struct XRibbonModel
{
    std::vector<XRibbonTab> tabs;
    std::vector<XRibbonBackstageItem> backstage;
    std::wstring backstageTitle;      // 左上标题，默认"文件"
    std::wstring backstageIcon;       // 左上返回箭头图标（相对 exe），空 = 内置矢量
    bool backstageOpen = false;       // 文件页是否展开（铺满整窗）
    int selected = 0;
    bool bodyCollapsed = false;   // 功能区是否收起（只留选项卡条），由调用方翻转

    const XRibbonTab* current() const
    {
        if (tabs.empty()) return nullptr;
        const int i = (selected < 0 || selected >= (int)tabs.size()) ? 0 : selected;
        return &tabs[(size_t)i];
    }
};

// ---------------------------------------------------------------- 尺寸
struct XRibbonMetrics
{
    int tabH    = 34;   // 选项卡条高
    int bodyH   = 92;   // 功能区高
    int btnW    = 58;   // 大按钮宽
    int btnH    = 60;   // 大按钮高
    int btnTop  = 4;
    int padX    = 12;   // 左内边距
    int divW    = 13;   // 组间分隔占位
    int titleTop = 68;  // 组名的 y（相对功能区）
    int titleH  = 16;
    int iconSide = 30;
    int smallW  = 104;  // 小按钮宽
    int smallH  = 20;   // 小按钮高（三层叠起来 = 大按钮高）
    int foldW   = 46;   // 折叠组的宽度（三个点 + 组名）
    int toggleW = 26;   // 选项卡条右端的收起/展开三角
};

// ---------------------------------------------------------------- 解析
inline bool ribbonTagIs(const std::wstring& tag, const wchar_t* a, const wchar_t* b)
{
    const std::wstring bare = xmlmfc::BareTag(tag);
    return _wcsicmp(bare.c_str(), a) == 0 || (b != nullptr && _wcsicmp(bare.c_str(), b) == 0);
}

inline std::wstring ribbonTextOf(XmlElement& n)
{
    std::wstring t = n.get(L"Header");
    if (t.empty()) t = n.get(L"Title");
    if (t.empty()) t = n.get(L"Text");
    return t;
}

inline void ribbonBackstageParse(XmlElement& node, XRibbonModel& m);

inline void ribbonParse(XmlElement& node, XRibbonModel& m)
{
    m.tabs.clear();
    // 结构每帧重建，但"当前选中页"与"收起状态"由调用方持有，不被 XML 覆盖
    const int keepSel = m.selected;
    const bool keepCollapsed = m.bodyCollapsed;

    for (auto& tabNode : node.children)
    {
        if (!ribbonTagIs(tabNode.tag, L"XRibbonTab", L"RibbonTab")) continue;
        XRibbonTab tab;
        tab.header = ribbonTextOf(tabNode);
        if (tab.header.empty()) tab.header = L"选项卡";

        for (auto& grpNode : tabNode.children)
        {
            if (!ribbonTagIs(grpNode.tag, L"XRibbonGroup", L"RibbonGroup")) continue;
            XRibbonGroup grp;
            grp.header = ribbonTextOf(grpNode);
            grp.dialog = grpNode.get(L"Dialog");

            for (auto& btnNode : grpNode.children)
            {
                if (!ribbonTagIs(btnNode.tag, L"XRibbonButton", L"RibbonButton")) continue;
                XRibbonButton btn;
                btn.name  = btnNode.name;
                btn.text  = ribbonTextOf(btnNode);
                btn.icon  = btnNode.get(L"Icon");
                btn.tiny = _wcsicmp(btnNode.get(L"Size").c_str(), L"Small") == 0;
                const std::wstring w = btnNode.get(L"Width");
                if (!w.empty()) btn.width = _wtoi(w.c_str());
                // 下拉项：<XRibbonMenuItem Name Text Icon/>
                for (auto& mi : btnNode.children)
                {
                    if (!ribbonTagIs(mi.tag, L"XRibbonMenuItem", L"RibbonMenuItem")) continue;
                    XRibbonButton::Item item;
                    item.name = mi.name;
                    item.text = ribbonTextOf(mi);
                    item.icon = mi.get(L"Icon");
                    btn.items.push_back(item);
                }
                grp.buttons.push_back(btn);
            }
            if (grp.buttons.empty()) continue;
            tab.groups.push_back(grp);
        }
        if (tab.groups.empty()) continue;
        m.tabs.push_back(tab);
    }

    const std::wstring sel = node.get(L"SelectedTab");
    m.selected = keepSel ? keepSel : (sel.empty() ? 0 : _wtoi(sel.c_str()));
    if (m.selected < 0 || m.selected >= (int)m.tabs.size()) m.selected = 0;
    m.bodyCollapsed = keepCollapsed;
    ribbonBackstageParse(node, m);
}

// 文件页（Backstage）：单独一段，节点是 <XRibbonBackstage>，子项 <XRibbonBackstageItem>
inline void ribbonBackstageParse(XmlElement& node, XRibbonModel& m)
{
    m.backstage.clear();
    m.backstageTitle.clear();
    for (auto& bs : node.children)
    {
        if (!ribbonTagIs(bs.tag, L"XRibbonBackstage", L"RibbonBackstage")) continue;
        m.backstageTitle = bs.get(L"Title");
        m.backstageIcon  = bs.get(L"Icon");
        for (auto& it : bs.children)
        {
            if (!ribbonTagIs(it.tag, L"XRibbonBackstageItem", L"RibbonBackstageItem")) continue;
            XRibbonBackstageItem item;
            item.name   = it.name;
            item.header = ribbonTextOf(it);
            item.text   = it.get(L"Text");
            item.icon   = it.get(L"Icon");
            if (item.header.empty()) item.header = item.text;
            m.backstage.push_back(item);
        }
        break;
    }
    if (m.backstage.empty()) m.backstageOpen = false;
}

// ---------------------------------------------------------------- 布局结果
struct XRibbonGroupBox
{
    RECT rc;                 // 整组矩形（含组名）
    RECT titleRc;            // 组名矩形
    RECT dialogRc = { 0, 0, 0, 0 };  // 组对话启动器（右下角斜箭头）
    std::vector<RECT> buttons;
    bool collapsed = false;  // 宽度不足时整组收成窄块（不裁掉）
};

// 组的自然宽度：大按钮各占一列，连续的小按钮每 3 个一列（三层叠起来）
inline int ribbonGroupWidth(const XRibbonGroup& g, const XRibbonMetrics& mx)
{
    int w = 0;
    int i = 0;
    const int n = (int)g.buttons.size();
    while (i < n)
    {
        if (!g.buttons[i].tiny)
        {
            w += (g.buttons[i].width > 0 ? g.buttons[i].width : mx.btnW);
            ++i;
        }
        else
        {
            int sub = 0;
            while (i < n && g.buttons[i].tiny) { ++sub; ++i; }
            w += ((sub + 2) / 3) * mx.smallW;
        }
    }
    return w;
}

// 组内每个按钮的矩形：大按钮一整块，小按钮按三层从 col 起向下排
inline std::vector<RECT> ribbonGroupRects(const XRibbonGroup& g, const XRibbonMetrics& mx,
                                           int left, int top)
{
    std::vector<RECT> out(g.buttons.size());
    int cx = left;
    int i = 0;
    const int n = (int)g.buttons.size();
    const int rowBase = top + mx.btnTop;
    while (i < n)
    {
        if (!g.buttons[i].tiny)
        {
            const int bw = g.buttons[i].width > 0 ? g.buttons[i].width : mx.btnW;
            out[(size_t)i] = RECT{ cx, rowBase, cx + bw, rowBase + mx.btnH };
            cx += bw;
            ++i;
        }
        else
        {
            int sub = 0;
            while (i < n && g.buttons[i].tiny)
            {
                const int col = sub / 3;
                const int row = sub % 3;
                const int x = cx + col * mx.smallW;
                const int y = rowBase + row * mx.smallH;
                out[(size_t)i] = RECT{ x, y, x + mx.smallW, y + mx.smallH };
                ++sub;
                ++i;
            }
            cx += ((sub + 2) / 3) * mx.smallW;
        }
    }
    return out;
}

struct XRibbonLayout
{
    std::vector<RECT> tabs;                  // 每个选项卡的矩形
    std::vector<XRibbonGroupBox> groups;     // 当前选项卡的组
    RECT toggleRc = { 0, 0, 0, 0 };          // 选项卡条右端的收起/展开三角
    int contentWidth = 0;                    // 功能区实际占用宽度
};

// 计算选项卡条 + 当前选项卡功能区的几何。
// bodyRc 给出功能区矩形（由调用方的行高决定），tabRc 给出选项卡条。
inline void ribbonLayout(const XRibbonModel& m, const XRibbonMetrics& mx,
                          const RECT& tabRc, const RECT& bodyRc, XRibbonLayout& out)
{
    out.tabs.clear();
    out.groups.clear();
    out.contentWidth = 0;

    // 选项卡：按文字宽度均分（最小 66），左对齐
    int x = tabRc.left + mx.padX;
    for (size_t i = 0; i < m.tabs.size(); ++i)
    {
        const int w = 66;
        out.tabs.push_back(RECT{ x, tabRc.top, x + w, tabRc.bottom });
        x += w;
    }

    // 选项卡条右端的收起/展开三角（Office 的"折叠功能区"按钮）
    out.toggleRc = RECT{ tabRc.right - mx.toggleW - 4, tabRc.top,
                         tabRc.right - 4, tabRc.bottom };

    // 功能区被收起时不做组布局（调用方已把行高压到选项卡条高度）
    if (m.bodyCollapsed || bodyRc.bottom <= bodyRc.top) return;

    const XRibbonTab* tab = m.current();
    if (tab == nullptr) return;

    // ---- 宽度不足时按"从右往左折叠"处理：整组收成一个窄块，不裁掉 ----
    const int avail = (bodyRc.right - mx.padX) - (bodyRc.left + mx.padX);
    std::vector<int> want(tab->groups.size());
    for (size_t gi = 0; gi < tab->groups.size(); ++gi)
        want[gi] = ribbonGroupWidth(tab->groups[gi], mx);

    std::vector<bool> folded(tab->groups.size(), false);
    auto totalWidth = [&]() -> int
    {
        int sum = 0;
        for (size_t gi = 0; gi < want.size(); ++gi)
        {
            const int w = folded[gi] ? mx.foldW : want[gi];
            sum += w + (gi + 1 < want.size() ? mx.divW : 0);
        }
        return sum;
    };
    // 从最后一组开始折叠，直到放得下；只剩一组时不再折叠（宁可留一点溢出也不空场）
    for (int gi = (int)want.size() - 1; gi >= 0 && totalWidth() > avail; --gi)
    {
        if ((int)want.size() - 1 < 1) break;
        if ((int)std::count(folded.begin(), folded.end(), true) >= (int)want.size() - 1) break;
        folded[(size_t)gi] = true;
    }

    int gx = bodyRc.left + mx.padX;
    for (size_t gi = 0; gi < tab->groups.size(); ++gi)
    {
        const XRibbonGroup& grp = tab->groups[gi];
        XRibbonGroupBox box;
        box.collapsed = folded[gi];

        if (box.collapsed)
        {
            box.rc = RECT{ gx, bodyRc.top, gx + mx.foldW, bodyRc.bottom };
            box.titleRc = RECT{ box.rc.left, bodyRc.top + mx.titleTop,
                                box.rc.right, bodyRc.top + mx.titleTop + mx.titleH };
        }
        else
        {
            box.rc = RECT{ gx, bodyRc.top, gx + want[gi], bodyRc.bottom };
            box.buttons = ribbonGroupRects(grp, mx, gx, bodyRc.top);
            box.titleRc = RECT{ box.rc.left, bodyRc.top + mx.titleTop,
                                box.rc.right, bodyRc.top + mx.titleTop + mx.titleH };
            // 组对话启动器（右下角斜箭头）
            if (!grp.dialog.empty())
                box.dialogRc = RECT{ box.rc.right - 18, bodyRc.top + mx.titleTop - 16,
                                     box.rc.right - 4, bodyRc.top + mx.titleTop };
        }

        out.groups.push_back(box);
        gx += (box.rc.right - box.rc.left) + mx.divW;
        out.contentWidth = gx - mx.divW - (bodyRc.left + mx.padX);
    }
}

// ---------------------------------------------------------------- 绘制
// 图标根目录（exe 目录）。三端启动时各设一次，库内负责把相对路径补成绝对路径，
// 这样 XML 里只写 res/new.png，设计器 / 预览器 / 运行时都指向自己的 res 目录。
inline std::wstring& ribbonIconBase()
{
    static std::wstring base;
    return base;
}

inline std::wstring ribbonResolveIcon(const std::wstring& icon)
{
    if (icon.empty()) return icon;
    if (icon.find(L':') != std::wstring::npos) return icon;   // 已是绝对路径
    const std::wstring& base = ribbonIconBase();
    return base.empty() ? icon : base + L"\\" + icon;
}

inline void ribbonDrawButton(XmlCanvas& cv, const RECT& rc, const XRibbonButton& btn,
                              bool hot, bool pressed, const XRibbonMetrics& mx, int ptSize)
{
    const ShellTheme& th = shell();
    const COLORREF back = pressed ? th.accentDim : (hot ? RGB(58, 70, 88) : RGB(45, 54, 66));
    // 左右各留 2px，相邻按钮才不会连成一条长块
    RECT face = { rc.left + 2, rc.top, rc.right - 2, rc.bottom };
    cv.fillRoundRect(face, back, back, 5);

    if (btn.icon.empty())
    {
        cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
        cv.text(rc, btn.text, hot ? th.text : th.textDim, DT_CENTER | DT_VCENTER, ptSize,
                std::wstring());
        cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
    }
    else
    {
        const int side = mx.iconSide;
        const int textH = ptSize + 12;
        const int contentH = side + 4 + textH;
        int top = rc.top + ((rc.bottom - rc.top) - contentH) / 2;
        if (top < rc.top + 4) top = rc.top + 4;
        const int cx = (rc.left + rc.right) / 2;
        RECT ir = { cx - side / 2, top, cx - side / 2 + side, top + side };
        cv.drawImage(ir, ribbonResolveIcon(btn.icon));

        if (!btn.text.empty())
        {
            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
            RECT tr = { rc.left + 2, top + side + 2, rc.right - 2, rc.bottom - 2 };
            cv.text(tr, btn.text, hot ? th.text : th.textDim, DT_CENTER, ptSize, std::wstring());
            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
        }
    }

    // 有下拉项：右下角画一个小三角（点这块弹菜单）
    if (!btn.items.empty())
    {
        const RECT d = ribbonDropdownRect(rc);
        const int cx = (d.left + d.right) / 2;
        const int cy = (d.top + d.bottom) / 2;
        const COLORREF c = hot ? th.text : th.textDim;
        cv.fillPath(std::vector<POINT>{ { cx - 4, cy - 2 }, { cx + 4, cy - 2 }, { cx, cy + 3 } }, c);
    }
}

inline void ribbonDrawTab(XmlCanvas& cv, const RECT& rc, const std::wstring& text,
                           bool selected, bool hot, int ptSize)
{
    const ShellTheme& th = shell();
    if (selected)
    {
        cv.fillRect(rc, RGB(38, 44, 54));
        cv.fillRect(RECT{ rc.left + 6, rc.bottom - 2, rc.right - 6, rc.bottom }, th.accent);
    }
    else if (hot)
    {
        cv.fillRect(rc, th.panelAlt);
    }
    cv.setTextStyle(selected ? L"SemiBold" : L"Normal", L"Normal", L"NoWrap", L"Center");
    cv.text(rc, text, selected ? RGB(255, 255, 255) : (hot ? th.text : th.textDim), DT_CENTER, ptSize, std::wstring());
    cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
}

// 小按钮：图标在左、文字在右，只占一行（三层叠起来与大按钮等高）
inline void ribbonDrawSmallButton(XmlCanvas& cv, const RECT& rc, const XRibbonButton& btn,
                                   bool hot, bool pressed, const XRibbonMetrics& mx, int ptSize)
{
    const ShellTheme& th = shell();
    (void)mx;
    RECT face = { rc.left + 1, rc.top, rc.right - 1, rc.bottom - 1 };
    if (pressed)
        cv.fillRoundRect(face, RGB(70, 86, 108), RGB(70, 86, 108), 3);
    else if (hot)
        cv.fillRoundRect(face, RGB(52, 62, 78), RGB(52, 62, 78), 3);

    const int cy = (face.top + face.bottom) / 2;
    if (!btn.icon.empty())
    {
        const std::wstring path = ribbonResolveIcon(btn.icon);
        RECT ic = { face.left + 4, cy - 8, face.left + 20, cy + 8 };
        cv.drawImage(ic, path);
    }
    RECT tr = { face.left + 24, face.top, face.right - 4, face.bottom };
    cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"VCenter");
    cv.text(tr, btn.text, (hot || pressed) ? th.text : th.textDim,
            DT_LEFT | DT_VCENTER, ptSize, std::wstring());
    cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
}

// 只画功能区（选项卡条以下）。正常绘制和"收起状态下临时浮出的功能区"共用这一段。
inline void ribbonDrawBody(XmlCanvas& cv, const RECT& bodyRc, const XRibbonModel& m,
                            const XRibbonMetrics& mx, const XRibbonLayout& lay,
                            const std::wstring& hotButton, const std::wstring& pressedButton,
                            int btnPt = 9)
{
    const ShellTheme& th = shell();
    cv.fillRect(bodyRc, RGB(32, 36, 44));
    cv.fillRect(RECT{ bodyRc.left, bodyRc.bottom - 1, bodyRc.right, bodyRc.bottom }, th.line);

    const XRibbonTab* tab = m.current();
    if (tab == nullptr) return;

    for (size_t gi = 0; gi < lay.groups.size() && gi < tab->groups.size(); ++gi)
    {
        const XRibbonGroupBox& box = lay.groups[gi];
        const XRibbonGroup& grp = tab->groups[gi];

        if (box.collapsed)
        {
            // 折叠组：一块窄面板 + 上方三个点（表示"还有内容"）+ 组名
            const bool hot = !hotButton.empty() &&
                             _wcsicmp(hotButton.c_str(), (std::wstring(L"ribbon:group:") +
                                                          std::to_wstring(gi)).c_str()) == 0;
            RECT face = { box.rc.left + 2, box.rc.top + mx.btnTop, box.rc.right - 2,
                          box.rc.top + mx.btnTop + mx.btnH };
            const COLORREF back = hot ? RGB(58, 70, 88) : RGB(45, 54, 66);
            cv.fillRoundRect(face, back, back, 5);

            const int cx = (face.left + face.right) / 2;
            const int cy = face.top + 16;
            for (int k = -1; k <= 1; ++k)
                cv.fillEllipse(RECT{ cx + k * 9 - 2, cy - 2, cx + k * 9 + 2, cy + 2 },
                               RGB(196, 205, 218), RGB(196, 205, 218));

            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
            cv.text(box.titleRc, grp.header, th.textDim, DT_CENTER | DT_VCENTER, 8, std::wstring());
            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
        }
        else
        {
            for (size_t bi = 0; bi < box.buttons.size() && bi < grp.buttons.size(); ++bi)
            {
                const XRibbonButton& b = grp.buttons[bi];
                const bool hot = !hotButton.empty() && _wcsicmp(hotButton.c_str(), b.name.c_str()) == 0;
                const bool press = !pressedButton.empty() && _wcsicmp(pressedButton.c_str(), b.name.c_str()) == 0;
                if (b.tiny)
                    ribbonDrawSmallButton(cv, box.buttons[bi], b, hot, press, mx, 8);
                else
                    ribbonDrawButton(cv, box.buttons[bi], b, hot, press, mx, btnPt);
            }

            // 组对话启动器：右下角斜箭头（点了由调用方开对话框）
            if (box.dialogRc.right > box.dialogRc.left)
            {
                const std::wstring dn = std::wstring(L"ribbon:dialog:") + std::to_wstring(gi);
                const bool dhot = !hotButton.empty() && _wcsicmp(hotButton.c_str(), dn.c_str()) == 0;
                const COLORREF c = dhot ? th.text : th.textDim;
                cv.fillRect(RECT{ box.dialogRc.left, box.dialogRc.bottom - 3,
                                  box.dialogRc.right, box.dialogRc.bottom - 1 }, c);
                cv.fillRect(RECT{ box.dialogRc.right - 3, box.dialogRc.top,
                                  box.dialogRc.right - 1, box.dialogRc.bottom }, c);
            }

            // 组名（居中暗字）
            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
            cv.text(box.titleRc, grp.header, th.textDim, DT_CENTER | DT_VCENTER, 8, std::wstring());
            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
        }

        // 组右分隔线
        const int lineX = box.rc.right + mx.divW / 2;
        if (gi + 1 < lay.groups.size())
            cv.fillRect(RECT{ lineX, bodyRc.top + 8, lineX + 1, bodyRc.top + mx.btnTop + mx.btnH },
                        RGB(58, 64, 76));
    }
}

// hotTab <0 表示无；hotButton 是按钮名（空 = 无悬停）；pressedButton 同理
inline void ribbonDraw(XmlCanvas& cv, const RECT& tabRc, const RECT& bodyRc, const XRibbonModel& m,
                        const XRibbonMetrics& mx, const XRibbonLayout& lay,
                        int hotTab, const std::wstring& hotButton, const std::wstring& pressedButton,
                        int tabPt = 10, int btnPt = 9)
{
    const ShellTheme& th = shell();

    // 选项卡条
    cv.fillRect(tabRc, RGB(35, 40, 48));
    cv.fillRect(RECT{ tabRc.left, tabRc.bottom - 1, tabRc.right, tabRc.bottom }, th.line);
    for (size_t i = 0; i < lay.tabs.size() && i < m.tabs.size(); ++i)
        ribbonDrawTab(cv, lay.tabs[i], m.tabs[i].header, (int)i == m.selected, (int)i == hotTab, tabPt);

    // 收起/展开三角：展开时朝下（点它收起），收起时朝上
    if (lay.toggleRc.right > lay.toggleRc.left)
    {
        const bool hot = _wcsicmp(hotButton.c_str(), L"ribbon:toggle") == 0;
        if (hot)
        {
            COLORREF back = RGB(52, 60, 72);
            cv.fillRoundRect(lay.toggleRc, back, back, 4);
        }
        const int cx = (lay.toggleRc.left + lay.toggleRc.right) / 2;
        const int cy = (lay.toggleRc.top + lay.toggleRc.bottom) / 2;
        const COLORREF c = hot ? th.text : th.textDim;
        if (m.bodyCollapsed)
            cv.fillPath({ POINT{ cx - 5, cy + 3 }, POINT{ cx + 5, cy + 3 }, POINT{ cx, cy - 4 } }, c);
        else
            cv.fillPath({ POINT{ cx - 5, cy - 3 }, POINT{ cx + 5, cy - 3 }, POINT{ cx, cy + 4 } }, c);
    }

    // 功能区（收起时不画）
    if (m.bodyCollapsed || bodyRc.bottom <= bodyRc.top) return;
    ribbonDrawBody(cv, bodyRc, m, mx, lay, hotButton, pressedButton, btnPt);
}

// ---------------------------------------------------------------- 命中
struct XRibbonHit
{
    int tab = -1;             // 命中的选项卡序号（-1 = 未命中）
    int group = -1;           // 命中的组序号
    int button = -1;          // 命中的按钮序号
    bool toggle = false;      // 命中收起/展开三角
};

inline XRibbonHit ribbonHitTest(const XRibbonLayout& lay, POINT pt)
{
    XRibbonHit hit;
    const RECT& tg = lay.toggleRc;
    if (tg.right > tg.left && pt.x >= tg.left && pt.x < tg.right && pt.y >= tg.top && pt.y < tg.bottom)
    {
        hit.toggle = true;
        return hit;
    }
    for (size_t i = 0; i < lay.tabs.size(); ++i)
    {
        const RECT& r = lay.tabs[i];
        if (pt.x >= r.left && pt.x < r.right && pt.y >= r.top && pt.y < r.bottom)
        {
            hit.tab = (int)i;
            return hit;
        }
    }
    for (size_t gi = 0; gi < lay.groups.size(); ++gi)
    {
        const XRibbonGroupBox& box = lay.groups[gi];
        if (box.collapsed)
        {
            const RECT& r = box.rc;
            if (pt.x >= r.left && pt.x < r.right && pt.y >= r.top && pt.y < r.bottom)
            {
                hit.group = (int)gi;   // button = -1：整组被折叠，调用方决定弹菜单
                return hit;
            }
            continue;
        }
        for (size_t bi = 0; bi < box.buttons.size(); ++bi)
        {
            const RECT& r = box.buttons[bi];
            if (pt.x >= r.left && pt.x < r.right && pt.y >= r.top && pt.y < r.bottom)
            {
                hit.group = (int)gi;
                hit.button = (int)bi;
                return hit;
            }
        }
    }
    return hit;
}

// ---------------------------------------------------------------- 折叠组溢出菜单
// 宽度不足被折成窄块的分组：点一下在块下方弹出该组的按钮列表（Office 的做法）。
struct XRibbonMenu
{
    bool visible = false;
    RECT rc = { 0, 0, 0, 0 };
    int tab = -1;                                  // 菜单属于哪个选项卡
    int group = -1;                                // 菜单属于哪个组
    std::wstring header;                           // 组名（菜单标题）
    std::vector<std::wstring> names;               // 每项按钮的 Name
    std::vector<std::wstring> texts;               // 每项按钮的 Text
    std::vector<std::wstring> icons;               // 每项按钮的 Icon
    std::vector<RECT> items;                       // 每项的矩形
};

inline void ribbonMenuLayout(const XRibbonModel& m, const XRibbonMetrics& mx, int tabIndex,
                              int group, const RECT& anchor, int rightLimit, XRibbonMenu& out)
{
    out.visible = false;
    out.items.clear();
    out.names.clear();
    out.texts.clear();
    out.icons.clear();

    if (tabIndex < 0 || tabIndex >= (int)m.tabs.size()) return;
    const XRibbonTab& t = m.tabs[(size_t)tabIndex];
    if (group < 0 || group >= (int)t.groups.size()) return;
    const XRibbonGroup& g = t.groups[(size_t)group];

    const int rowH = 32;
    int w = 150;
    for (const auto& b : g.buttons)
    {
        const int need = 14 + 18 + 10 + (int)b.text.size() * 13;
        if (need > w) w = need;
    }
    int x = anchor.left;
    if (rightLimit > 0 && x + w > rightLimit - 4) x = rightLimit - 4 - w;
    if (x < 4) x = 4;
    const int y = anchor.bottom + 2;
    const int h = 6 + (int)g.buttons.size() * rowH + 6;

    out.rc = RECT{ x, y, x + w, y + h };
    out.visible = true;
    out.tab = tabIndex;
    out.group = group;
    out.header = g.header;

    int iy = y + 6;
    for (const auto& b : g.buttons)
    {
        out.items.push_back(RECT{ x + 4, iy, x + w - 4, iy + rowH });
        out.names.push_back(b.name);
        out.texts.push_back(b.text.empty() ? b.name : b.text);
        out.icons.push_back(b.icon);
        iy += rowH;
    }
    (void)mx;
}

// 通用：按给定的项清单排一个下拉菜单（按钮的 split/dropdown、组溢出都走这里）
inline void ribbonMenuLayoutItems(const std::vector<std::wstring>& names,
                                   const std::vector<std::wstring>& texts,
                                   const std::vector<std::wstring>& icons,
                                   const RECT& anchor, int rightLimit, XRibbonMenu& out,
                                   int tabIndex = -1)
{
    out.visible = false;
    out.items.clear();
    out.names = names;
    out.texts = texts;
    out.icons = icons;
    out.tab = tabIndex;
    if (names.empty()) return;

    const int rowH = 32;
    int w = 150;
    for (const auto& t : texts)
    {
        const int need = 14 + 18 + 10 + (int)t.size() * 13;
        if (need > w) w = need;
    }
    int x = anchor.left;
    if (rightLimit > 0 && x + w > rightLimit - 4) x = rightLimit - 4 - w;
    if (x < 4) x = 4;
    const int y = anchor.bottom + 2;
    const int h = 6 + (int)names.size() * rowH + 6;

    out.rc = RECT{ x, y, x + w, y + h };
    out.visible = true;
    int iy = y + 6;
    for (size_t i = 0; i < names.size(); ++i)
    {
        out.items.push_back(RECT{ x + 4, iy, x + w - 4, iy + rowH });
        iy += rowH;
    }
}

inline int ribbonMenuHit(const XRibbonMenu& menu, POINT pt)
{
    if (!menu.visible) return -1;
    const RECT& r = menu.rc;
    if (pt.x < r.left || pt.x >= r.right || pt.y < r.top || pt.y >= r.bottom) return -1;
    for (size_t i = 0; i < menu.items.size(); ++i)
    {
        const RECT& it = menu.items[i];
        if (pt.x >= it.left && pt.x < it.right && pt.y >= it.top && pt.y < it.bottom) return (int)i;
    }
    return -2;   // 在菜单内但不在任何项上
}

inline void ribbonMenuDraw(XmlCanvas& cv, const XRibbonMenu& menu, int hotItem, int ptSize = 9)
{
    if (!menu.visible) return;
    const ShellTheme& th = shell();

    const RECT& rc = menu.rc;
    RECT shadow = { rc.left + 2, rc.top + 3, rc.right + 2, rc.bottom + 3 };
    cv.fillRoundRect(shadow, RGB(16, 19, 24), RGB(16, 19, 24), 6);
    cv.fillRoundRect(rc, RGB(40, 46, 56), RGB(74, 82, 96), 6);

    for (size_t i = 0; i < menu.items.size(); ++i)
    {
        const RECT& it = menu.items[i];
        if ((int)i == hotItem)
        {
            COLORREF back = RGB(58, 70, 88);
            cv.fillRoundRect(it, back, back, 4);
        }

        int tx = it.left + 6;
        if (i < menu.icons.size() && !menu.icons[i].empty())
        {
            const std::wstring path = ribbonResolveIcon(menu.icons[i]);
            RECT ic = { it.left + 6, (it.top + it.bottom) / 2 - 9, it.left + 24, (it.top + it.bottom) / 2 + 9 };
            cv.drawImage(ic, path);
            tx = it.left + 30;
        }
        RECT tr = { tx, it.top, it.right - 6, it.bottom };
        cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"VCenter");
        cv.text(tr, i < menu.texts.size() ? menu.texts[i] : std::wstring(), th.text,
                DT_LEFT | DT_VCENTER, ptSize, std::wstring());
        cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
    }
}

// ---------------------------------------------------------------- 文件页（Backstage）
// 占满整个宿主矩形：左侧深色导航列（顶部返回 + 项列表），右侧内容页。
struct XRibbonBackstageLayout
{
    RECT rc      = { 0, 0, 0, 0 };
    RECT navRc   = { 0, 0, 0, 0 };
    RECT closeRc = { 0, 0, 0, 0 };   // 左上"←"返回
    RECT contentRc = { 0, 0, 0, 0 };
};

inline int ribbonBackstageNavW() { return 210; }

inline XRibbonBackstageLayout ribbonBackstageLayout(XRibbonModel& m, const RECT& host)
{
    XRibbonBackstageLayout lay;
    lay.rc = host;
    lay.navRc = RECT{ host.left, host.top, host.left + ribbonBackstageNavW(), host.bottom };
    lay.closeRc = RECT{ lay.navRc.left + 10, lay.navRc.top + 8, lay.navRc.left + 38, lay.navRc.top + 36 };
    lay.contentRc = RECT{ lay.navRc.right, host.top, host.right, host.bottom };

    int y = lay.navRc.top + 52;
    for (auto& it : m.backstage)
    {
        it.rc = RECT{ lay.navRc.left + 8, y, lay.navRc.right - 8, y + 44 };
        y += 46;
    }
    return lay;
}

inline void ribbonBackstageDraw(XmlCanvas& cv, const XRibbonModel& m, const XRibbonBackstageLayout& lay,
                                 int hotItem, bool hotClose, int ptSize)
{
    const ShellTheme& th = shell();

    // 整块底 + 左列
    cv.fillRect(lay.rc, th.base);
    cv.fillRect(lay.navRc, th.panel);
    cv.fillRect(RECT{ lay.navRc.right - 1, lay.navRc.top, lay.navRc.right, lay.navRc.bottom }, th.line);

    // 返回箭头：优先用 XML 指的图标（抗锯齿、线条细），没有才用内置矢量
    {
        RECT a = lay.closeRc;
        if (hotClose)
            cv.fillRoundRect(RECT{ a.left - 4, a.top - 4, a.right + 4, a.bottom + 4 },
                             RGB(45, 54, 66), RGB(45, 54, 66), 4);
        if (!m.backstageIcon.empty())
        {
            const std::wstring path = ribbonResolveIcon(m.backstageIcon);
            RECT ic = { a.left + 4, a.top + 4, a.right - 4, a.bottom - 4 };
            cv.drawImage(ic, path);
        }
        else
        {
            const COLORREF c = hotClose ? th.text : th.textDim;
            const int cx = a.left + 8;
            const int cy = (a.top + a.bottom) / 2;
            const int len = 14;
            cv.strokeLine({ cx, cy }, { cx + len, cy }, c, 2);              // 横
            cv.strokeLine({ cx, cy }, { cx + 7, cy - 7 }, c, 2);            // 上斜
            cv.strokeLine({ cx, cy }, { cx + 7, cy + 7 }, c, 2);            // 下斜
        }
    }
    {
        RECT tr = { lay.closeRc.right + 6, lay.closeRc.top, lay.navRc.right - 8, lay.closeRc.bottom };
        cv.text(tr, m.backstageTitle.empty() ? std::wstring(L"文件") : m.backstageTitle,
                th.text, DT_LEFT | DT_VCENTER, ptSize + 2, std::wstring());
    }

    // 项：可带图标（XML 里 Icon="res\new.png"）
    for (size_t i = 0; i < m.backstage.size(); ++i)
    {
        const RECT& it = m.backstage[i].rc;
        if (hotItem == (int)i)
            cv.fillRect(it, th.panelAlt);
        int tx = it.left + 14;
        if (!m.backstage[i].icon.empty())
        {
            const int side = 18;
            const int cyi = (it.top + it.bottom) / 2;
            RECT ic = { tx, cyi - side / 2, tx + side, cyi + side / 2 };
            cv.drawImage(ic, ribbonResolveIcon(m.backstage[i].icon));
            tx += side + 10;
        }
        RECT tr = { tx, it.top, it.right - 6, it.bottom };
        cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"VCenter");
        cv.text(tr, m.backstage[i].header, hotItem == (int)i ? th.text : th.textDim,
                DT_LEFT | DT_VCENTER, ptSize, std::wstring());
        cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
    }

    // 右侧内容：(hotItem 有则显示该项说明)
    {
        const int idx = (hotItem >= 0 && hotItem < (int)m.backstage.size()) ? hotItem
                        : (m.backstage.empty() ? -1 : 0);
        RECT tr = { lay.contentRc.left + 48, lay.contentRc.top + 56,
                    lay.contentRc.right - 48, lay.contentRc.top + 96 };
        const std::wstring title = idx >= 0 ? m.backstage[(size_t)idx].header
                                     : (m.backstageTitle.empty() ? std::wstring(L"文件") : m.backstageTitle);
        cv.text(tr, title, th.text, DT_LEFT | DT_VCENTER, ptSize + 10, std::wstring());
        if (idx >= 0 && !m.backstage[(size_t)idx].text.empty())
        {
            RECT d = { tr.left, tr.bottom + 4, tr.right, tr.bottom + 4 + 40 };
            cv.text(d, m.backstage[(size_t)idx].text, th.textDim, DT_LEFT | DT_TOP | DT_WORDBREAK,
                    ptSize + 1, std::wstring());
        }
    }
}

// 命中：>=0 项序号；-1 空白（点空白也当返回）；-2 返回按钮
inline int ribbonBackstageHit(const XRibbonModel& m, const XRibbonBackstageLayout& lay, POINT pt)
{
    if (PtInRect(&lay.closeRc, pt)) return -2;
    for (size_t i = 0; i < m.backstage.size(); ++i)
        if (PtInRect(&m.backstage[i].rc, pt)) return (int)i;
    return -1;
}

}  // namespace xmlctl

