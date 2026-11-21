// XmlFlexPanel.cpp -- control-level flex container (CSS semantics).
// NOTE: ASCII-only source to avoid codepage issues; comments in English by design.

#include "XmlFlexPanel.h"

#include <algorithm>
#include <commctrl.h>
#include <cwctype>
#include <cstdlib>
#include <cstdio>

#pragma comment(lib, "comctl32.lib")

namespace
{
    // WM_SIZE hook: the container re-lays out its own children when the host resizes the
    // window (MoveWindow / SetWindowPos). No whole-tree reflowXml involved.
    LRESULT CALLBACK FlexPanelSizeProc(HWND h, UINT msg, WPARAM wp, LPARAM lp,
                                       UINT_PTR /*sub*/ , DWORD_PTR ref)
    {
        // 真父子层级：容器的子控件（以及嵌套容器）把 WM_CTLCOLOR* 发给本容器，
        // 容器自己是 STATIC，DefWindowProc 会给系统白刷子 —— 那正是"整块变白"的来源。
        // 子控件的 owner-draw（SS_OWNERDRAW 的 WM_DRAWITEM / WM_MEASUREITEM）和命令通知
        // 同样发给父窗口而不是宿主，容器不转发就只剩一片空底色、文字全丢。
        // 一律沿父链上抛，最后由宿主统一处理（与 SkinBox 的转发同一策略）。
        if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLORBTN ||
            msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX ||
            msg == WM_DRAWITEM || msg == WM_MEASUREITEM ||
            msg == WM_COMMAND || msg == WM_NOTIFY)
        {
            if (const HWND p = ::GetParent(h); p != nullptr)
                return ::SendMessageW(p, msg, wp, lp);
        }
        const LRESULT r = DefSubclassProc(h, msg, wp, lp);
        // 重排时机 = 尺寸变化 + 位置变化：
        // 子窗口坐标是按"容器原点 + 相对矩形"拼出来的绝对坐标，容器自己一挪，
        // 子项就必须重拼一次，否则尺寸没变时（WM_SIZE 不发）子项会停在旧位置。
        if ((msg == WM_SIZE || msg == WM_MOVE || msg == WM_WINDOWPOSCHANGED) && ref != 0)
        {
            XmlFlexPanel* self = reinterpret_cast<XmlFlexPanel*>(ref);
            self->reflowItems();
        }
        return r;
    }

    // "flex-direction" / "flexDirection" / "flex_direction" -> "flexdirection"
    std::wstring NormKey(const wchar_t* name)
    {
        std::wstring out;
        if (name == nullptr)
            return out;
        for (const wchar_t* p = name; *p != L'\0'; ++p)
        {
            if (*p == L'-' || *p == L'_' || *p == L' ')
                continue;
            out.push_back(static_cast<wchar_t>(towlower(*p)));
        }
        return out;
    }

    // split numbers by space / comma / semicolon (negative numbers allowed)
    std::vector<int> Numbers(const std::wstring& v)
    {
        std::vector<int> out;
        int cur = 0;
        bool has = false;
        bool neg = false;
        for (size_t i = 0; i <= v.size(); ++i)
        {
            const wchar_t c = (i < v.size()) ? v[i] : L' ';
            if (c == L'-' && !has)
            {
                neg = true;
                continue;
            }
            if (c >= L'0' && c <= L'9')
            {
                cur = cur * 10 + static_cast<int>(c - L'0');
                has = true;
                continue;
            }
            if (has)
            {
                out.push_back(neg ? -cur : cur);
                cur = 0;
                has = false;
                neg = false;
            }
            else
            {
                neg = false;
            }
        }
        return out;
    }

    bool Is(const std::wstring& key, const wchar_t* a)
    {
        return key == a;
    }

    bool IsAny(const std::wstring& key, const wchar_t* a, const wchar_t* b)
    {
        return key == a || key == b;
    }
}

void XmlFlexPanel::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
    // Take the real HWND so a size change repaints the children from the container itself.
    if (HWND hw = static_cast<HWND>(host->getDlgItemHandle(id)))
        SetWindowSubclass(hw, FlexPanelSizeProc, 1, reinterpret_cast<DWORD_PTR>(this));
}

void XmlFlexPanel::pushBackdrop()
{
    // 容器的背景（div 的 background）走宿主绘制节点：宿主按 id 登记一块背板，
    // 之后 moveControl 会自动跟随几何，不需要容器自己 WM_PAINT。
    if (m_host == nullptr || m_id < 1000 || m_id > 65000)
        return;
    int x = 0, y = 0, w = 0, h = 0;
    m_host->getControlRect(m_id, x, y, w, h);
    m_host->applyContainerBackdrop(m_id, x, y, w, h, m_ap);
}

void XmlFlexPanel::setBounds(int x, int y, int w, int h)
{
    XmlMfcControlBase::setBounds(x, y, w, h);
    // Control-level reflow: a size change alone re-lays out children; the host does not
    // need to run a whole-tree reflowXml for this container.
    reflowItems();
    pushBackdrop();      // 容器改尺寸后背板矩形要跟上
}

int XmlFlexPanel::mainMargin(const XmlFlexItem& it) const
{
    return m_horizontal ? (it.marginL + it.marginR) : (it.marginT + it.marginB);
}

int XmlFlexPanel::crossMargin(const XmlFlexItem& it) const
{
    return m_horizontal ? (it.marginT + it.marginB) : (it.marginL + it.marginR);
}

int XmlFlexPanel::mainSizeOf(const XmlFlexItem& it, int naturalW, int naturalH) const
{
    if (it.basis >= 0)
        return it.basis;
    return m_horizontal ? naturalW : naturalH;
}

int XmlFlexPanel::crossSizeOf(const XmlFlexItem& it, int naturalW, int naturalH) const
{
    if (it.crossSize >= 0)
        return it.crossSize;
    return m_horizontal ? naturalH : naturalW;
}

std::vector<XmlFlexRect> XmlFlexPanel::computeLayout(int frameW, int frameH,
                                                     const std::vector<XmlFlexItem>& items) const
{
    std::vector<XmlFlexRect> result;
    const bool horizontal = (m_dir == XmlFlexDir::Row || m_dir == XmlFlexDir::RowReverse);
    const bool reverseMain = (m_dir == XmlFlexDir::RowReverse || m_dir == XmlFlexDir::ColumnReverse);

    const int contentW = (std::max)(0, frameW - m_padL - m_padR);
    const int contentH = (std::max)(0, frameH - m_padT - m_padB);
    const int mainAvail = horizontal ? contentW : contentH;
    const int crossAvail = horizontal ? contentH : contentW;

    struct Box
    {
        int id = 0;
        int mainSize = 0;
        int crossSize = 0;
        int mMainLead = 0;
        int mMainTrail = 0;
        int mCrossLead = 0;
        int mCrossTrail = 0;
        // CSS margin:auto：主轴方向上的 auto 边距优先吃掉剩余空间（再轮到 justify-content）
        bool autoMainLead = false;
        bool autoMainTrail = false;
        // 交叉轴同理：column 容器里 `margin-left:auto` 就是水平居中
        bool autoCrossLead = false;
        bool autoCrossTrail = false;
        int selfAlign = -1;
        float grow = 0.0f;
        float shrink = 1.0f;
        int mainPos = 0;
        int crossPos = 0;
        bool crossDeclared = false;   // 交叉轴尺寸已显式声明（百分比或 px），stretch 不再改写它
        int minMain = -1;             // CSS min-width / min-height（按主轴投影到 main/cross）
        int maxMain = -1;
        int minCross = -1;
        int maxCross = -1;
        int baseline = -1;            // 顶到文字基线（align-items:baseline 用）
    };

    std::vector<Box> boxes;
    boxes.reserve(items.size());
    for (const XmlFlexItem& it : items)
    {
        if (it.absolute)
            continue;
        Box b;
        b.id = it.id;
        const int naturalMain = horizontal ? it.naturalW : it.naturalH;
        const int naturalCross = horizontal ? it.naturalH : it.naturalW;
        b.mainSize = (std::max)(0, it.basis >= 0 ? it.basis : naturalMain);
        b.crossSize = (std::max)(0, it.crossSize >= 0 ? it.crossSize : naturalCross);
        // CSS 百分比：basis 用百分比时按容器内容盒的主轴尺寸换算（html 的 width:50% 就是这个意思）
        if (it.basisPct >= 0.0f)
            b.mainSize = (std::max)(0, static_cast<int>(mainAvail * it.basisPct / 100.0f));
        if (it.crossPct >= 0.0f)
            b.crossSize = (std::max)(0, static_cast<int>(crossAvail * it.crossPct / 100.0f));
        b.crossDeclared = (it.crossPct >= 0.0f || it.crossSize >= 0);
        b.mMainLead = horizontal ? it.marginL : it.marginT;
        b.mMainTrail = horizontal ? it.marginR : it.marginB;
        b.mCrossLead = horizontal ? it.marginT : it.marginL;
        b.mCrossTrail = horizontal ? it.marginB : it.marginR;
        b.autoMainLead = horizontal ? it.marginLAuto : it.marginTAuto;
        b.autoMainTrail = horizontal ? it.marginRAuto : it.marginBAuto;
        b.autoCrossLead = horizontal ? it.marginTAuto : it.marginLAuto;
        b.autoCrossTrail = horizontal ? it.marginBAuto : it.marginRAuto;
        b.selfAlign = it.selfAlign;
        b.grow = it.grow;
        b.shrink = it.shrink;
        // CSS 尺寸上下限按主轴/交叉轴投影：row 时 width→main、height→cross，column 反之
        b.minMain = horizontal ? it.minW : it.minH;
        b.maxMain = horizontal ? it.maxW : it.maxH;
        b.minCross = horizontal ? it.minH : it.minW;
        b.maxCross = horizontal ? it.maxH : it.maxW;
        b.baseline = it.baseline;
        if (b.minMain >= 0 && b.mainSize < b.minMain)
            b.mainSize = b.minMain;
        if (b.maxMain >= 0 && b.mainSize > b.maxMain)
            b.mainSize = b.maxMain;
        if (b.minCross >= 0 && b.crossSize < b.minCross)
            b.crossSize = b.minCross;
        if (b.maxCross >= 0 && b.crossSize > b.maxCross)
            b.crossSize = b.maxCross;
        boxes.push_back(b);
    }
    if (boxes.empty())
        return result;

    const int gapMain = horizontal ? m_gapX : m_gapY;
    const int gapCross = horizontal ? m_gapY : m_gapX;

    // ---- line breaking (flex-wrap) ----
    struct Line
    {
        int first = 0;
        int count = 0;
        int mainUsed = 0;
        int crossSize = 0;
    };

    std::vector<Line> lines;
    {
        Line cur;
        for (size_t i = 0; i < boxes.size(); ++i)
        {
            const Box& b = boxes[i];
            const int outer = b.mainSize + b.mMainLead + b.mMainTrail;
            const int need = outer + (cur.count > 0 ? gapMain : 0);
            if (m_wrap != XmlFlexWrap::NoWrap && cur.count > 0 && cur.mainUsed + need > mainAvail)
            {
                lines.push_back(cur);
                cur = Line();
                cur.first = static_cast<int>(i);
                cur.count = 1;
                cur.mainUsed = outer;
                cur.crossSize = b.crossSize + b.mCrossLead + b.mCrossTrail;
                continue;
            }
            if (cur.count == 0)
                cur.first = static_cast<int>(i);
            cur.mainUsed += need;
            cur.count++;
            const int cross = b.crossSize + b.mCrossLead + b.mCrossTrail;
            if (cross > cur.crossSize)
                cur.crossSize = cross;
        }
        if (cur.count > 0)
            lines.push_back(cur);
    }

    // ---- main axis: grow / shrink, then justify-content ----
    for (size_t li = 0; li < lines.size(); ++li)
    {
        Line& line = lines[li];
        int base = 0;
        float totalGrow = 0.0f;
        float totalShrinkWeight = 0.0f;
        for (int k = 0; k < line.count; ++k)
        {
            const Box& b = boxes[line.first + k];
            base += b.mainSize + b.mMainLead + b.mMainTrail;
            totalGrow += b.grow;
            totalShrinkWeight += b.shrink > 0.0f ? b.shrink * static_cast<float>(b.mainSize) : 0.0f;
        }
        base += gapMain * (line.count - 1);

        int freeMain = mainAvail - base;
        if (freeMain > 0 && totalGrow > 0.0f)
        {
            // CSS flex-grow 是「多轮分配」：某一项被 max-width 卡住后，它让出来的空间
            // 要回到池子里再分给其它项。只 clamp 一次会把余量凭空吞掉
            // （表现为 max-width:120 + flex:1 的项 120，兄弟却拿不到剩下的 77）。
            int pool = freeMain;
            for (int round = 0; round < 4 && pool > 0; ++round)
            {
                float g = 0.0f;
                for (int k = 0; k < line.count; ++k)
                {
                    const Box& b = boxes[line.first + k];
                    if (b.grow <= 0.0f)
                        continue;
                    if (b.maxMain >= 0 && b.mainSize >= b.maxMain)
                        continue;
                    g += b.grow;
                }
                if (g <= 0.0f)
                    break;
                int given = 0;
                for (int k = 0; k < line.count; ++k)
                {
                    Box& b = boxes[line.first + k];
                    if (b.grow <= 0.0f)
                        continue;
                    if (b.maxMain >= 0 && b.mainSize >= b.maxMain)
                        continue;
                    int add = static_cast<int>(static_cast<float>(pool) * (b.grow / g));
                    if (b.maxMain >= 0 && b.mainSize + add > b.maxMain)
                        add = b.maxMain - b.mainSize;
                    if (add < 0)
                        add = 0;
                    b.mainSize += add;
                    given += add;
                }
                if (given == 0)
                    break;
                pool -= given;
            }
            // 舍入余数给最后一个还有余量的 grow 项（老引擎规则）
            if (pool > 0)
            {
                for (int k = line.count - 1; k >= 0; --k)
                {
                    Box& b = boxes[line.first + k];
                    if (b.grow <= 0.0f)
                        continue;
                    if (b.maxMain >= 0)
                    {
                        const int room = b.maxMain - b.mainSize;
                        if (room <= 0)
                            continue;
                        b.mainSize += (pool < room ? pool : room);
                    }
                    else
                        b.mainSize += pool;
                    break;
                }
            }
        }
        else if (freeMain < 0 && totalShrinkWeight > 0.0f)
        {
            const int overflow = -freeMain;
            for (int k = 0; k < line.count; ++k)
            {
                Box& b = boxes[line.first + k];
                if (b.shrink <= 0.0f)
                    continue;
                const float w = b.shrink * static_cast<float>(b.mainSize);
                const int cut = static_cast<int>(static_cast<float>(overflow) * (w / totalShrinkWeight));
                b.mainSize = (std::max)(0, b.mainSize - cut);
                // CSS：shrink 不许压到 min-width 以下（卡片在窄屏里不该被压成一条线）
                if (b.minMain >= 0 && b.mainSize < b.minMain)
                    b.mainSize = b.minMain;
            }
        }

        int used = gapMain * (line.count - 1);
        for (int k = 0; k < line.count; ++k)
        {
            const Box& b = boxes[line.first + k];
            used += b.mainSize + b.mMainLead + b.mMainTrail;
        }
        freeMain = mainAvail - used;
        if (freeMain < 0)
            freeMain = 0;

        // CSS margin:auto —— 主轴上的 auto 边距先按"每边均分"吃掉全部剩余空间，
        // 剩下的（通常只剩舍入余数）才交给 justify-content。这就是 HTML 里
        // `margin-left:auto` 把按钮顶到右端、`margin:0 auto` 让块居中的原理。
        int autoCount = 0;
        for (int k = 0; k < line.count; ++k)
        {
            const Box& b = boxes[line.first + k];
            if (b.autoMainLead)
                ++autoCount;
            if (b.autoMainTrail)
                ++autoCount;
        }
        if (autoCount > 0 && freeMain > 0)
        {
            const int each = freeMain / autoCount;
            for (int k = 0; k < line.count; ++k)
            {
                Box& b = boxes[line.first + k];
                if (b.autoMainLead)
                    b.mMainLead += each;
                if (b.autoMainTrail)
                    b.mMainTrail += each;
            }
            freeMain = 0;
        }

        int lead = 0;
        int extra = 0;
        switch (m_justify)
        {
        case XmlFlexJustify::Center:
            lead = freeMain / 2;
            break;
        case XmlFlexJustify::End:
            lead = freeMain;
            break;
        case XmlFlexJustify::Between:
            extra = (line.count > 1) ? (freeMain / (line.count - 1)) : 0;
            break;
        case XmlFlexJustify::Around:
            extra = freeMain / line.count;
            lead = extra / 2;
            break;
        case XmlFlexJustify::Evenly:
            extra = freeMain / (line.count + 1);
            lead = extra;
            break;
        default:
            break;
        }

        int pos = lead;
        for (int k = 0; k < line.count; ++k)
        {
            Box& b = boxes[line.first + k];
            pos += b.mMainLead;
            b.mainPos = reverseMain ? (mainAvail - (pos + b.mainSize)) : pos;
            pos += b.mainSize + b.mMainTrail + gapMain + extra;
        }
    }

    // ---- cross axis: align-items / align-self, line stacking, wrap-reverse ----
    // CSS align-content：多行容器里把"行组"整体在交叉轴分布（单行容器无意义）。
    // 此前只有单行语义，flex-wrap:wrap + align-content:center 仍从容器顶部堆行。
    int acStart = 0, acExtra = 0;
    if (m_wrap != XmlFlexWrap::NoWrap && lines.size() > 1)
    {
        int usedCross = gapCross * static_cast<int>(lines.size() - 1);
        for (const Line& l : lines)
            usedCross += l.crossSize;
        int freeCross = (std::max)(0, crossAvail - usedCross);
        const int n = static_cast<int>(lines.size());
        switch (m_alignContent)
        {
        case XmlFlexJustify::Center:
            acStart = freeCross / 2;
            break;
        case XmlFlexJustify::End:
            acStart = freeCross;
            break;
        case XmlFlexJustify::Between:
            if (n > 1)
                acExtra = freeCross / (n - 1);
            break;
        case XmlFlexJustify::Around:
            acStart = freeCross / (2 * n);
            acExtra = freeCross / n;
            break;
        case XmlFlexJustify::Evenly:
            acStart = freeCross / (n + 1);
            acExtra = freeCross / (n + 1);
            break;
        default:
            break;
        }
        // CSS align-content:stretch：剩余交叉空间均分给各行（行高变大），行内交叉轴
        // 尺寸为 auto 的子项随后由 align-items:stretch 拉满整行；写死 cross 尺寸的
        // 子项不被拉伸。早先 m_acStretch 只在解析/回读时登记，布局里从没读过。
        if (m_acStretch && freeCross > 0 && n > 0)
        {
            const int share = freeCross / n;
            const int rem = freeCross - share * n;
            for (size_t li = 0; li < lines.size(); ++li)
            {
                lines[li].crossSize += share;
                if (li == lines.size() - 1)
                    lines[li].crossSize += rem;
            }
        }
    }
    int crossCursor = acStart;
    for (size_t li = 0; li < lines.size(); ++li)
    {
        const Line& line = lines[li];
        int lineCross = line.crossSize;
        // single-line container: the leftover cross space belongs to that line
        // (this is what CSS align-items:stretch does for a one-line flex box)
        if (lines.size() == 1 && m_wrap == XmlFlexWrap::NoWrap)
            lineCross = (std::max)(lineCross, crossAvail);
        else if (lineCross <= 0)
            lineCross = crossAvail;

        // CSS align-items:baseline —— 行内所有文本项的"文字基线"要落在同一条线上，
        // 不是各自的顶边对齐。先求出本行最大的"顶到基线"距离，再据此下移其它项。
        int lineMaxBaseline = -1;
        for (int k = 0; k < line.count; ++k)
        {
            const Box& b = boxes[line.first + k];
            const int eff = (b.selfAlign >= 0) ? b.selfAlign : static_cast<int>(m_align);
            if (static_cast<XmlFlexAlign>(eff) != XmlFlexAlign::Baseline)
                continue;
            if (b.baseline >= 0 && b.mCrossLead + b.baseline > lineMaxBaseline)
                lineMaxBaseline = b.mCrossLead + b.baseline;
        }

        for (int k = 0; k < line.count; ++k)
        {
            Box& b = boxes[line.first + k];
            const int eff = (b.selfAlign >= 0) ? b.selfAlign : static_cast<int>(m_align);
            // CSS：交叉轴上的 auto 边距优先于 align-items —— 把剩余交叉空间均分给 auto 边，
            // 于是 column 容器里 `margin-left:auto;margin-right:auto` 就等于水平居中。
            if (b.autoCrossLead || b.autoCrossTrail)
            {
                int freeCross = lineCross - (b.crossSize + b.mCrossLead + b.mCrossTrail);
                if (freeCross > 0)
                {
                    int cnt = (b.autoCrossLead ? 1 : 0) + (b.autoCrossTrail ? 1 : 0);
                    const int each = freeCross / cnt;
                    if (b.autoCrossLead)
                        b.mCrossLead += each;
                    if (b.autoCrossTrail)
                        b.mCrossTrail += each;
                }
                b.crossPos = crossCursor + b.mCrossLead;
                continue;
            }
            switch (static_cast<XmlFlexAlign>(eff))
            {
            case XmlFlexAlign::Center:
                b.crossPos = crossCursor +
                             (lineCross - (b.crossSize + b.mCrossLead + b.mCrossTrail)) / 2 +
                             b.mCrossLead;
                break;
            case XmlFlexAlign::End:
                b.crossPos = crossCursor + lineCross - b.crossSize - b.mCrossTrail;
                break;
            case XmlFlexAlign::Baseline:
                // 没有文字基线的项（图片/空容器）退回顶边，浏览器也是这么兜底的
                b.crossPos = crossCursor + (lineMaxBaseline >= 0
                                                ? lineMaxBaseline - b.baseline
                                                : 0) +
                             (b.baseline >= 0 ? 0 : b.mCrossLead);
                break;
            case XmlFlexAlign::Stretch:
                b.crossPos = crossCursor + b.mCrossLead;
                // CSS：align-items:stretch 只对"尺寸为 auto"的子项生效，写了 height/width 的不拉满；
                // 且拉满后仍受 min-/max- 约束（浏览器不会为了 stretch 突破 max-height）
                if (!b.crossDeclared)
                {
                    b.crossSize = (std::max)(0, lineCross - b.mCrossLead - b.mCrossTrail);
                    if (b.minCross >= 0 && b.crossSize < b.minCross)
                        b.crossSize = b.minCross;
                    if (b.maxCross >= 0 && b.crossSize > b.maxCross)
                        b.crossSize = b.maxCross;
                }
                break;
            default: // Start
                b.crossPos = crossCursor + b.mCrossLead;
                break;
            }
        }
        crossCursor += lineCross + gapCross + acExtra;
    }

    if (m_wrap == XmlFlexWrap::WrapReverse)
    {
        const int totalCross = (std::max)(0, crossCursor - gapCross - acExtra);
        for (size_t i = 0; i < boxes.size(); ++i)
            boxes[i].crossPos = totalCross - boxes[i].crossPos - boxes[i].crossSize;
    }

    // TEMP PROBE (43): dump box sizes
    if (GetEnvironmentVariableW(L"PYARX_FLEX_DBG", nullptr, 0) != 0)
    {
        wchar_t tmp[MAX_PATH] = {0};
        GetTempPathW(MAX_PATH, tmp);
        std::wstring log = std::wstring(tmp) + L"flexpanel.log";
        FILE* f = _wfopen(log.c_str(), L"a");
        if (f != nullptr)
        {
            fwprintf(f, L"[cl] id=%d hor=%d frame=(%d,%d) avail=(%d,%d) n=%d lines=%d\n",
                     m_id, horizontal ? 1 : 0, frameW, frameH, mainAvail, crossAvail,
                     (int)boxes.size(), (int)lines.size());
            for (size_t i = 0; i < boxes.size(); ++i)
                fwprintf(f, L"  [b%02d] id=%d main=%d cross=%d decl=%d self=%d grow=%.1f minM=%d maxM=%d\n",
                         (int)i, boxes[i].id, boxes[i].mainSize, boxes[i].crossSize,
                         boxes[i].crossDeclared ? 1 : 0, boxes[i].selfAlign, boxes[i].grow,
                         boxes[i].minMain, boxes[i].maxMain);
            fclose(f);
        }
    }

    // ---- emit rects relative to the container's top-left (content box offset added) ----
    result.reserve(boxes.size());
    for (size_t i = 0; i < boxes.size(); ++i)
    {
        const Box& b = boxes[i];
        XmlFlexRect r;
        r.x = m_padL + (horizontal ? b.mainPos : b.crossPos);
        r.y = m_padT + (horizontal ? b.crossPos : b.mainPos);
        r.w = (std::max)(1, horizontal ? b.mainSize : b.crossSize);
        r.h = (std::max)(1, horizontal ? b.crossSize : b.mainSize);
        result.push_back(r);
    }
    return result;
}

int XmlFlexPanel::measureDesiredCross(int frameW, int frameH,
                                       const std::vector<XmlFlexItem>& items) const
{
    if (items.empty())
        return 0;
    const bool horizontal = (m_dir == XmlFlexDir::Row || m_dir == XmlFlexDir::RowReverse);
    const int contentW = (std::max)(0, frameW - m_padL - m_padR);
    const int contentH = (std::max)(0, frameH - m_padT - m_padB);
    // column + wrap 拆列必须有显式可用高：没有高度约束时 HTML 也不拆列，这里不伪造列数。
    if (!horizontal && contentH <= 0)
        return 0;
    const int mainAvail = horizontal ? contentW : contentH;

    struct MBox
    {
        int mainSize = 0;
        int crossSize = 0;
        int mMainLead = 0, mMainTrail = 0;
        int mCrossLead = 0, mCrossTrail = 0;
    };
    std::vector<MBox> boxes;
    for (const XmlFlexItem& it : items)
    {
        if (it.absolute)
            continue;
        MBox b;
        const int naturalMain = horizontal ? it.naturalW : it.naturalH;
        const int naturalCross = horizontal ? it.naturalH : it.naturalW;
        b.mainSize = (std::max)(0, it.basis >= 0 ? it.basis : naturalMain);
        b.crossSize = (std::max)(0, it.crossSize >= 0 ? it.crossSize : naturalCross);
        if (it.basisPct >= 0.0f)
            b.mainSize = (std::max)(0, static_cast<int>(mainAvail * it.basisPct / 100.0f));
        b.mMainLead = horizontal ? it.marginL : it.marginT;
        b.mMainTrail = horizontal ? it.marginR : it.marginB;
        b.mCrossLead = horizontal ? it.marginT : it.marginL;
        b.mCrossTrail = horizontal ? it.marginB : it.marginR;
        const int minMain = horizontal ? it.minW : it.minH;
        const int maxMain = horizontal ? it.maxW : it.maxH;
        const int minCross = horizontal ? it.minH : it.minW;
        const int maxCross = horizontal ? it.maxH : it.maxW;
        if (minMain >= 0 && b.mainSize < minMain) b.mainSize = minMain;
        if (maxMain >= 0 && b.mainSize > maxMain) b.mainSize = maxMain;
        if (minCross >= 0 && b.crossSize < minCross) b.crossSize = minCross;
        if (maxCross >= 0 && b.crossSize > maxCross) b.crossSize = maxCross;
        boxes.push_back(b);
    }
    if (boxes.empty())
        return 0;

    // 与 computeLayout 同一套换行口径：主轴装不下就另起一行（列）。
    // gap 分工：column 方向 gapMain=row-gap 管列内项距（纵向），gapCross=column-gap 管列距（横向）。
    const int gapMain = horizontal ? m_gapX : m_gapY;
    const int gapCross = horizontal ? m_gapY : m_gapX;
    struct Line
    {
        int first = 0, count = 0, mainUsed = 0, crossSize = 0;
    };
    std::vector<Line> lines;
    Line cur;
    for (size_t i = 0; i < boxes.size(); ++i)
    {
        const MBox& b = boxes[i];
        const int outer = b.mainSize + b.mMainLead + b.mMainTrail;
        const int need = outer + (cur.count > 0 ? gapMain : 0);
        if (m_wrap != XmlFlexWrap::NoWrap && cur.count > 0 && cur.mainUsed + need > mainAvail)
        {
            lines.push_back(cur);
            cur = Line();
            cur.first = static_cast<int>(i);
            cur.count = 1;
            cur.mainUsed = outer;
            cur.crossSize = b.crossSize + b.mCrossLead + b.mCrossTrail;
            continue;
        }
        if (cur.count == 0)
            cur.first = static_cast<int>(i);
        cur.mainUsed += need;
        cur.count++;
        const int cross = b.crossSize + b.mCrossLead + b.mCrossTrail;
        if (cross > cur.crossSize)
            cur.crossSize = cross;
    }
    if (cur.count > 0)
        lines.push_back(cur);

    int usedCross = gapCross * static_cast<int>(lines.size() - 1);
    for (const Line& l : lines)
        usedCross += l.crossSize;
    return usedCross;
}

void XmlFlexPanel::reflowItems()
{
    // TEMP probe: PYARX_FLEX_DBG=1 -> %TEMP%\flexpanel.log
    static const bool dbg = (GetEnvironmentVariableW(L"PYARX_FLEX_DBG", nullptr, 0) != 0);
    if (dbg)
    {
        wchar_t tmp[MAX_PATH] = {0};
        GetTempPathW(MAX_PATH, tmp);
        std::wstring log = std::wstring(tmp) + L"flexpanel.log";
        FILE* f = _wfopen(log.c_str(), L"a");
        if (f != nullptr)
        {
            int dx = 0, dy = 0, dw = 0, dh = 0;
            if (m_host != nullptr)
                m_host->getControlRect(m_id, dx, dy, dw, dh);
            fwprintf(f, L"reflow id=%d host=%d items=%d rect=(%d,%d,%d,%d)\n",
                     m_id, m_host != nullptr ? 1 : 0, (int)m_items.size(), dx, dy, dw, dh);
            fclose(f);
        }
    }
    if (m_host == nullptr || m_id <= 0 || m_items.empty())
        return;

    int cx = 0, cy = 0, cw = 0, ch = 0;
    // 统一走宿主的几何入口：宿主内部是「GetWindowRect + ScreenToClient(宿主)」，
    // 也就是"屏幕坐标一次换算到宿主客户区" —— 与嵌套层数无关，容器嵌几层都对。
    // 以前这里自己再 MapWindowPoints 到**直接父窗口**，就少算了一层（子项全部叠到页面左上角）。
    m_host->getControlRect(m_id, cx, cy, cw, ch);
    if (cw <= 0 || ch <= 0)
        return;

    std::vector<XmlFlexRect> rects = computeLayout(cw, ch, m_items);

    // commit: the container moves the child windows itself
    applyRects(rects);
}

// 把外部（XmlCore 适配层）算好的一次布局结果落到真实子窗口：决策仍在容器侧，
// 适配层只递坐标。absolute 子项不参与 flex 流，按"非 absolute 顺序"对齐 rects。
void XmlFlexPanel::applyRects(const std::vector<XmlFlexRect>& rects)
{
    if (m_host == nullptr || m_id <= 0 || rects.empty())
        return;
    int cx = 0, cy = 0, cw = 0, ch = 0;
    m_host->getControlRect(m_id, cx, cy, cw, ch);
    if (cw <= 0 || ch <= 0)
        return;
    size_t k = 0;
    for (size_t i = 0; i < m_items.size() && k < rects.size(); ++i)
    {
        if (m_items[i].absolute)
            continue;
        const int id = m_items[i].id;
        if (id > 0)
            m_host->moveControl(id, cx + rects[k].x, cy + rects[k].y, rects[k].w, rects[k].h);
        ++k;
    }
}

bool XmlFlexPanel::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
    const std::wstring key = NormKey(name);
    const std::wstring val = value;
    std::wstring low;
    for (wchar_t c : val)
        low.push_back(static_cast<wchar_t>(towlower(c)));
    // strip separators for enum matching ("space-between" -> "spacebetween")
    std::wstring token;
    for (wchar_t c : low)
    {
        if (c == L' ' || c == L'-' || c == L'_')
            continue;
        token.push_back(c);
    }

    if (IsAny(key, L"flexdirection", L"direction") || Is(key, L"flex"))
    {
        if (token == L"rowreverse")
            m_dir = XmlFlexDir::RowReverse;
        else if (token == L"columnreverse")
            m_dir = XmlFlexDir::ColumnReverse;
        else if (token == L"column")
            m_dir = XmlFlexDir::Column;
        else
            m_dir = XmlFlexDir::Row;
        m_horizontal = (m_dir == XmlFlexDir::Row || m_dir == XmlFlexDir::RowReverse);
        return true;
    }
    if (IsAny(key, L"flexwrap", L"wrap"))
    {
        if (token == L"wrapreverse")
            m_wrap = XmlFlexWrap::WrapReverse;
        else if (token == L"wrap")
            m_wrap = XmlFlexWrap::Wrap;
        else
            m_wrap = XmlFlexWrap::NoWrap;
        return true;
    }
    if (IsAny(key, L"justifycontent", L"justify"))
    {
        if (token == L"center")
            m_justify = XmlFlexJustify::Center;
        else if (token == L"end" || token == L"flexend" || token == L"right")
            m_justify = XmlFlexJustify::End;
        else if (token == L"between" || token == L"spacebetween")
            m_justify = XmlFlexJustify::Between;
        else if (token == L"around" || token == L"spacearound")
            m_justify = XmlFlexJustify::Around;
        else if (token == L"evenly" || token == L"spaceevenly")
            m_justify = XmlFlexJustify::Evenly;
        else
            m_justify = XmlFlexJustify::Start;
        return true;
    }
    if (Is(key, L"aligncontent"))
    {
        // CSS align-content：多行时的行组分布，值域与 justify-content 同形
        m_acStretch = false;
        if (token == L"center")
            m_alignContent = XmlFlexJustify::Center;
        else if (token == L"end" || token == L"flexend")
            m_alignContent = XmlFlexJustify::End;
        else if (token == L"between" || token == L"spacebetween")
            m_alignContent = XmlFlexJustify::Between;
        else if (token == L"around" || token == L"spacearound")
            m_alignContent = XmlFlexJustify::Around;
        else if (token == L"evenly" || token == L"spaceevenly")
            m_alignContent = XmlFlexJustify::Evenly;
        else
            m_alignContent = XmlFlexJustify::Start;
        if (token == L"stretch")
            m_acStretch = true;
        return true;
    }
    if (IsAny(key, L"alignitems", L"align"))
    {
        if (token == L"center")
            m_align = XmlFlexAlign::Center;
        else if (token == L"end" || token == L"flexend")
            m_align = XmlFlexAlign::End;
        else if (token == L"baseline")
            m_align = XmlFlexAlign::Baseline;
        else if (token == L"stretch")
            m_align = XmlFlexAlign::Stretch;
        else
            m_align = XmlFlexAlign::Start;
        return true;
    }
    if (Is(key, L"gap"))
    {
        const std::vector<int> n = Numbers(val);
        if (n.size() >= 2)
        {
            m_gapY = n[0];
            m_gapX = n[1];
        }
        else if (n.size() == 1)
        {
            m_gapY = m_gapX = n[0];
        }
        return true;
    }
    if (Is(key, L"rowgap"))
    {
        const std::vector<int> n = Numbers(val);
        if (!n.empty())
            m_gapY = n[0];
        return true;
    }
    if (Is(key, L"columngap"))
    {
        const std::vector<int> n = Numbers(val);
        if (!n.empty())
            m_gapX = n[0];
        return true;
    }
    if (Is(key, L"padding"))
    {
        const std::vector<int> n = Numbers(val);
        // 属性顺序与 XmlCore ParsePadding 一致（WPF：左 上 右 下），
        // 不能按 CSS「上 右 下 左」读，否则两套解析器把同一个 Padding 读反。
        if (n.size() == 1)
        {
            m_padL = m_padT = m_padR = m_padB = n[0];
        }
        else if (n.size() == 2)
        {
            m_padL = m_padR = n[0];
            m_padT = m_padB = n[1];
        }
        else if (n.size() >= 4)
        {
            m_padL = n[0];
            m_padT = n[1];
            m_padR = n[2];
            m_padB = n[3];
        }
        return true;
    }
    return false;
}

std::wstring XmlFlexPanel::getOwnProperty(const wchar_t* name) const
{
    const std::wstring key = NormKey(name);
    if (IsAny(key, L"flexdirection", L"direction") || Is(key, L"flex"))
    {
        switch (m_dir)
        {
        case XmlFlexDir::RowReverse:
            return L"row-reverse";
        case XmlFlexDir::Column:
            return L"column";
        case XmlFlexDir::ColumnReverse:
            return L"column-reverse";
        default:
            return L"row";
        }
    }
    if (IsAny(key, L"flexwrap", L"wrap"))
    {
        switch (m_wrap)
        {
        case XmlFlexWrap::Wrap:
            return L"wrap";
        case XmlFlexWrap::WrapReverse:
            return L"wrap-reverse";
        default:
            return L"nowrap";
        }
    }
    if (IsAny(key, L"justifycontent", L"justify"))
    {
        switch (m_justify)
        {
        case XmlFlexJustify::Center:
            return L"center";
        case XmlFlexJustify::End:
            return L"end";
        case XmlFlexJustify::Between:
            return L"space-between";
        case XmlFlexJustify::Around:
            return L"space-around";
        case XmlFlexJustify::Evenly:
            return L"space-evenly";
        default:
            return L"start";
        }
    }
    if (Is(key, L"aligncontent"))
    {
        if (m_acStretch)
            return L"stretch";
        switch (m_alignContent)
        {
        case XmlFlexJustify::Center:
            return L"center";
        case XmlFlexJustify::End:
            return L"flex-end";
        case XmlFlexJustify::Between:
            return L"space-between";
        case XmlFlexJustify::Around:
            return L"space-around";
        case XmlFlexJustify::Evenly:
            return L"space-evenly";
        default:
            return L"start";
        }
    }
    if (IsAny(key, L"alignitems", L"align"))
    {
        switch (m_align)
        {
        case XmlFlexAlign::Center:
            return L"center";
        case XmlFlexAlign::End:
            return L"end";
        case XmlFlexAlign::Stretch:
            return L"stretch";
        case XmlFlexAlign::Baseline:
            return L"baseline";
        default:
            return L"start";
        }
    }
    if (Is(key, L"gap"))
    {
        if (m_gapX == m_gapY)
            return std::to_wstring(m_gapX);
        return std::to_wstring(m_gapY) + L" " + std::to_wstring(m_gapX);
    }
    if (Is(key, L"rowgap"))
        return std::to_wstring(m_gapY);
    if (Is(key, L"columngap"))
        return std::to_wstring(m_gapX);
    if (Is(key, L"padding"))
        return std::to_wstring(m_padT) + L" " + std::to_wstring(m_padR) + L" " +
               std::to_wstring(m_padB) + L" " + std::to_wstring(m_padL);
    return std::wstring();
}

std::vector<XmlProperty> XmlFlexPanel::ownProperties() const
{
    return {};
}
