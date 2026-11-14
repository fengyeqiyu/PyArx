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

#include "XmlControls.h"
#include "XmlCore.h"

namespace xmlctl {

// ---------------------------------------------------------------- 数据模型
struct XRibbonButton
{
    std::wstring name;          // 动作名（与设计器命令同名，如 btnPaste）
    std::wstring text;          // 按钮下方标题
    std::wstring icon;          // PNG 路径（相对 exe）
    int width  = 58;
};

struct XRibbonGroup
{
    std::wstring header;        // 组名（底部小字）
    std::vector<XRibbonButton> buttons;
};

struct XRibbonTab
{
    std::wstring header;
    std::vector<XRibbonGroup> groups;
};

struct XRibbonModel
{
    std::vector<XRibbonTab> tabs;
    int selected = 0;

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

inline void ribbonParse(XmlElement& node, XRibbonModel& m)
{
    m.tabs.clear();
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

            for (auto& btnNode : grpNode.children)
            {
                if (!ribbonTagIs(btnNode.tag, L"XRibbonButton", L"RibbonButton")) continue;
                XRibbonButton btn;
                btn.name  = btnNode.name;
                btn.text  = ribbonTextOf(btnNode);
                btn.icon  = btnNode.get(L"Icon");
                const std::wstring w = btnNode.get(L"Width");
                if (!w.empty()) btn.width = _wtoi(w.c_str());
                grp.buttons.push_back(btn);
            }
            if (grp.buttons.empty()) continue;
            tab.groups.push_back(grp);
        }
        if (tab.groups.empty()) continue;
        m.tabs.push_back(tab);
    }

    const std::wstring sel = node.get(L"SelectedTab");
    m.selected = sel.empty() ? 0 : _wtoi(sel.c_str());
}

// ---------------------------------------------------------------- 布局结果
struct XRibbonGroupBox
{
    RECT rc;                 // 整组矩形（含组名）
    RECT titleRc;            // 组名矩形
    std::vector<RECT> buttons;
};

struct XRibbonLayout
{
    std::vector<RECT> tabs;                  // 每个选项卡的矩形
    std::vector<XRibbonGroupBox> groups;     // 当前选项卡的组
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

    const XRibbonTab* tab = m.current();
    if (tab == nullptr) return;

    int gx = bodyRc.left + mx.padX;
    for (size_t gi = 0; gi < tab->groups.size(); ++gi)
    {
        const XRibbonGroup& grp = tab->groups[gi];
        XRibbonGroupBox box;
        box.rc = RECT{ gx, bodyRc.top, gx + (int)grp.buttons.size() * mx.btnW, bodyRc.bottom };

        int bx = gx;
        for (size_t bi = 0; bi < grp.buttons.size(); ++bi)
        {
            const int bw = grp.buttons[bi].width > 0 ? grp.buttons[bi].width : mx.btnW;
            box.buttons.push_back(RECT{ bx, bodyRc.top + mx.btnTop, bx + bw, bodyRc.top + mx.btnTop + mx.btnH });
            bx += bw;
        }

        box.titleRc = RECT{ box.rc.left, bodyRc.top + mx.titleTop,
                            box.rc.right, bodyRc.top + mx.titleTop + mx.titleH };
        out.groups.push_back(box);
        gx += bx - gx;
        out.contentWidth = gx - (bodyRc.left + mx.padX);
        gx += mx.divW;
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
        return;
    }

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

    // 功能区
    cv.fillRect(bodyRc, RGB(32, 36, 44));
    cv.fillRect(RECT{ bodyRc.left, bodyRc.bottom - 1, bodyRc.right, bodyRc.bottom }, th.line);

    const XRibbonTab* tab = m.current();
    if (tab == nullptr) return;

    for (size_t gi = 0; gi < lay.groups.size() && gi < tab->groups.size(); ++gi)
    {
        const XRibbonGroupBox& box = lay.groups[gi];
        const XRibbonGroup& grp = tab->groups[gi];

        for (size_t bi = 0; bi < box.buttons.size() && bi < grp.buttons.size(); ++bi)
        {
            const XRibbonButton& b = grp.buttons[bi];
            const bool hot = !hotButton.empty() && _wcsicmp(hotButton.c_str(), b.name.c_str()) == 0;
            const bool press = !pressedButton.empty() && _wcsicmp(pressedButton.c_str(), b.name.c_str()) == 0;
            ribbonDrawButton(cv, box.buttons[bi], b, hot, press, mx, btnPt);
        }

        // 组名（居中暗字）
        cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
        cv.text(box.titleRc, grp.header, th.textDim, DT_CENTER | DT_VCENTER, 8, std::wstring());
        cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");

        // 组右分隔线
        const int lineX = box.rc.right + mx.divW / 2;
        if (gi + 1 < lay.groups.size())
            cv.fillRect(RECT{ lineX, bodyRc.top + 8, lineX + 1, bodyRc.top + mx.btnTop + mx.btnH },
                        RGB(58, 64, 76));
    }
}

// ---------------------------------------------------------------- 命中
struct XRibbonHit
{
    int tab = -1;             // 命中的选项卡序号（-1 = 未命中）
    int group = -1;           // 命中的组序号
    int button = -1;          // 命中的按钮序号
};

inline XRibbonHit ribbonHitTest(const XRibbonLayout& lay, POINT pt)
{
    XRibbonHit hit;
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

}  // namespace xmlctl
