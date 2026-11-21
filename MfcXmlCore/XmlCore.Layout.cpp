// XmlCore.Engine.cpp —— 拆自 XmlCore.cpp 第 115-3494 行。
#include "XmlCoreCommon.h"
#include "XmlCoreInternal.h"

// ===========================================================================
// 工具
// ===========================================================================
namespace xmlcore_impl
{
    // CSS flex 布局：HTML 的默认排版方式，在 XML 里用 Display="flex" 表达。
    // 主轴（Flex/Gap/Justify/Grow + 反向）与交叉轴（Align/Self/stretch）按
    // flexbox 模型计算；绝对定位、inset:0 撑满父的子项不排队，仍走原布局逻辑。
    // ===================================================================
    void LayoutNode(XmlElement& node, XmlFlow& flow);          // 前置声明
    void FlexLayoutChild(XmlElement& child, int px, int py, int pw, int ph);

    // flex 子项在"没声明宽度"时的自然宽：HTML 里是 width:auto，由内容撑开。
    // 方向很关键：横排容器的宽 = 子项宽之和 + gap；竖排容器的宽 = 最宽的那个子项。
    // 把竖排容器也按"求和"算，整排卡片会被撑成三倍宽、直接溢出到窗口外。
    int FlexContentWidth(const XmlElement& node, int depth, int gapFallback)
    {
        if (depth > 8)
            return 120;
        const bool rowDir = node.flex ? (node.flexDir == 0 || node.flexDir == 1) : node.horizontal;
        const int gap = rowDir ? (node.flexGapX > 0 ? node.flexGapX : gapFallback)
                               : (node.flexGapY > 0 ? node.flexGapY : gapFallback);
        int sumW = 0;
        int maxW = 0;
        int count = 0;
        for (const auto& child : node.children)
        {
            const std::wstring tag = KindOf(child.tag);
            if (tag == L"rowdefinition" || tag == L"columndefinition" || child.templateChild)
                continue;
            int w = 0;
            if (child.hasW)
                w = child.w;
            else if (IsContainer(tag))
                w = FlexContentWidth(child, depth + 1, gap);
            else
                w = NaturalWidth(tag);
            w += child.marginL + child.marginR;
            sumW += w;
            if (w > maxW)
                maxW = w;
            ++count;
        }
        int total = rowDir ? sumW : maxW;
        if (rowDir && count > 1)
            total += gap * (count - 1);
        total += node.padL + node.padR;
        return (std::max)(24, total);
    }

    // 控件层 flex 容器的标签：这些标签的排版由 XmlFlexPanel 自己的算法负责，
    // XmlCore 只把 XML 属性翻译成容器参数（P0：flex 落在底层控件里）。
    // <row>/<column> 是 HTML 式写法：写了标签就是横排/竖排容器，不用再写 direction。
    bool IsFlexPanelElement(const XmlElement& node)
    {
        const std::wstring kind = KindOf(node.tag);
        return kind == L"flexpanel" || kind == L"flexbox" ||
               kind == L"row" || kind == L"column";
    }

    // CSS `display:none` / HTML `hidden` 的元素在 HTML 里**完全不占位**：
    // 布局阶段要把它从 flex 项里剔掉（控件本身仍建出来，脚本仍能引用）。
    bool ElHidden(const XmlElement& e)
    {
        std::wstring v = e.get(L"Visible");
        std::wstring d = e.get(L"Display");
        for (auto* p : {&v, &d})
        {
            size_t a = 0, b = p->size();
            while (a < b && iswspace((*p)[a])) ++a;
            while (b > a && iswspace((*p)[b - 1])) --b;
            *p = p->substr(a, b - a);
            for (auto& ch : *p) ch = (wchar_t)towlower(ch);
        }
        if (!v.empty() && (v == L"false" || v == L"0" || v == L"off" || v == L"no"))
            return true;
        return d == L"none";
    }

    void LayoutFlex(XmlElement& node, XmlFlow& flow)
    {
        const int cx = flow.offsetX + node.padL;
        const int cy = flow.offsetY + node.padT;
        const int cw = (std::max)(1, flow.width - node.padL - node.padR);
        const int ch = (std::max)(1, (flow.height > 0 ? flow.height : 0) - node.padT - node.padB);

        // 控件层 flex 容器：标签是 flexpanel/flexbox 就走控件自己的算法，
        // 属性（flex-direction / flex-wrap / justify-content / align-items / gap）现翻进算法实例。
        const bool flexPanelTag = IsFlexPanelElement(node);
        XmlFlexPanel panelImpl;
        if (flexPanelTag)
        {
            // <column> 天生竖排、<row> 天生横排（HTML 写法），显式 direction 属性仍可覆盖
            if (KindOf(node.tag) == L"column")
                panelImpl.setDirection(XmlFlexDir::Column);
            for (const auto& kv : node.attrs)
                panelImpl.setOwnProperty(LocalName(kv.first).c_str(), kv.second);
            // 没写 gap 时沿用 Spacing，老写法在容器里一样生效
            if (panelImpl.rowGap() <= 0 && panelImpl.columnGap() <= 0 && node.spacing > 0)
                panelImpl.setGap(node.spacing, node.spacing);
        }
        const bool row = flexPanelTag
                             ? (panelImpl.direction() == XmlFlexDir::Row ||
                                panelImpl.direction() == XmlFlexDir::RowReverse)
                             : (node.flexDir == 0 || node.flexDir == 1);
        const bool reverse = (node.flexDir == 1 || node.flexDir == 3);
        // 没写 Gap 就沿用 Spacing：老写法（Spacing="8"）在 flex 容器里也直观生效
        const int gapMain = row ? (node.flexGapX > 0 ? node.flexGapX : node.spacing)
                                : (node.flexGapY > 0 ? node.flexGapY : node.spacing);
        const int mainSize = row ? cw : ch;
        const int crossSize = row ? ch : cw;

        struct FlexItem
        {
            XmlElement* el = nullptr;
            int main = 0, cross = 0;
            int mStart = 0, mEnd = 0, cStart = 0, cEnd = 0;
            // CSS margin:auto：数值按 0，但主轴方向要先吃掉剩余空间
            bool autoStart = false, autoEnd = false;
            bool autoCStart = false, autoCEnd = false;
            float grow = 0.0f;
            int self = -1;
        };
        std::vector<FlexItem> items;
        std::vector<XmlFlexItem> panelItems;   // 控件层：同一批子项的 flex 参数
        std::vector<XmlElement*> freeOnes;

        // 容器内"绝对定位"子项（写了 left/top/right/bottom 之一）：它们不参与 flex 主轴，
        // 但自己的几何必须由这里落位 —— 早前只在 items 分支里由 FlexLayoutChild 设置
        // preview，freeOnes 直接交给 LayoutNode 只排了它们的子树，自身 preview 恒为 0，
        // 于是 div 里 `<label style="left:10;width:160;height:32">` 建出来的窗口是 0×0。
        // CSS 语义：left/top 相对容器内框（已扣 padding），未写的一边按 content 尺寸贴边，
        // 两条边都写则尺寸由两条边反推。
        auto layoutFree = [&](XmlElement* a)
        {
            // 撑满父（inset:0）走自己的语义（占内框一块），不能按 left/top 直接盖掉
            if (a->fillParent)
            {
                XmlFlow f2;
                f2.margin = 0;
                f2.spacing = a->spacing;
                f2.padding = 0;
                f2.paddingY = 0;
                f2.width = cw;
                f2.height = ch;
                f2.offsetX = cx;
                f2.offsetY = cy;
                f2.cursorX = 0;
                f2.cursorY = 0;
                f2.horizontal = a->hasOrientation ? a->horizontal : false;
                LayoutNode(*a, f2);
                return;
            }
            const std::wstring eR = a->get(L"Right");
            const std::wstring eB = a->get(L"Bottom");
            int fx = a->hasX ? a->x : 0;
            int fy = a->hasY ? a->y : 0;
            int fw = 0, fh = 0;
            if (a->pctW > 0.0f && a->pctW < 100.0f)
                fw = (std::max)(1, static_cast<int>(cw * a->pctW / 100.0f + 0.5f));
            else if (a->hasW)
                fw = a->w;
            else
                fw = (std::max)(1, cw - fx);
            if (a->pctH > 0.0f && a->pctH < 100.0f)
                fh = (std::max)(1, static_cast<int>(ch * a->pctH / 100.0f + 0.5f));
            else if (a->hasH)
                fh = a->h;
            else
                fh = (std::max)(20, DefaultHeight(KindOf(a->tag)));
            if (!a->hasX && !eR.empty())
                fx = (std::max)(0, cw - CssEdgePx(eR, cw, 0) - fw);
            if (!a->hasY && !eB.empty())
                fy = (std::max)(0, ch - CssEdgePx(eB, ch, 0) - fh);
            a->relX = fx;
            a->relY = fy;
            a->relW = fw;
            a->relH = fh;
            a->previewX = cx + fx;
            a->previewY = cy + fy;
            a->previewW = fw;
            a->previewH = fh;
            XmlFlow inner;
            inner.margin = 0;
            inner.spacing = a->spacing;
            inner.padding = a->padL;
            inner.paddingY = a->padT;
            inner.width = fw;
            inner.height = fh;
            inner.offsetX = a->previewX;
            inner.offsetY = a->previewY;
            inner.cursorX = a->padL;
            inner.cursorY = a->padT;
            inner.horizontal = a->hasOrientation ? a->horizontal : false;
            LayoutNode(*a, inner);
        };

        // CSS order：主轴上的视觉顺序，与文档顺序无关。HTML 里这是少数"不改语义只改次序"
        // 的能力（把主按钮排到最后、把角标排到最前）；相同 order 保持文档顺序（规范要求稳定排序）。
        auto flexOrderOf = [](const XmlElement& e) -> int {
            const std::wstring v = e.get(L"Order");
            return v.empty() ? 0 : static_cast<int>(wcstol(v.c_str(), nullptr, 10));
        };
        std::vector<XmlElement*> ordered;
        ordered.reserve(node.children.size());
        for (auto& c : node.children)
            ordered.push_back(&c);
        std::stable_sort(ordered.begin(), ordered.end(),
                         [&flexOrderOf](XmlElement* a, XmlElement* b) {
                             return flexOrderOf(*a) < flexOrderOf(*b);
                         });

        for (XmlElement* cp : ordered)
        {
            XmlElement& child = *cp;
            const std::wstring tag = KindOf(child.tag);
            if (tag == L"rowdefinition" || tag == L"columndefinition" || child.templateChild)
                continue;
            if (ElHidden(child))
                continue;   // display:none / hidden 不占位（HTML 语义）
            // 只写 right/bottom（未写 left/top）同样是绝对定位：CSS 里这类盒脱离 flex 主轴，
            // 尺寸不够时由对面那条边决定位置。把它们和 left/top 一起当 free 项处理。
            const std::wstring onlyR = child.get(L"Right");
            const std::wstring onlyB = child.get(L"Bottom");
            if (child.fillParent || child.hasX || child.hasY ||
                !onlyR.empty() || !onlyB.empty())
            {
                freeOnes.push_back(&child);
                continue;
            }
            FlexItem it;
            it.el = &child;
            if (row)
            {
                it.mStart = child.marginL; it.mEnd = child.marginR;
                it.cStart = child.marginT; it.cEnd = child.marginB;
                it.autoStart = child.marginLAuto; it.autoEnd = child.marginRAuto;
                it.autoCStart = child.marginTAuto; it.autoCEnd = child.marginBAuto;
            }
            else
            {
                it.mStart = child.marginT; it.mEnd = child.marginB;
                it.cStart = child.marginL; it.cEnd = child.marginR;
                it.autoStart = child.marginTAuto; it.autoEnd = child.marginBAuto;
                it.autoCStart = child.marginLAuto; it.autoCEnd = child.marginRAuto;
            }
            it.grow = child.flexGrow;
            it.self = child.flexSelf;

            const bool hasMain = row ? child.hasW : child.hasH;
            const bool hasCross = row ? child.hasH : child.hasW;
            if (hasMain)
                it.main = row ? child.w : child.h;
            else if (IsContainer(tag) || IsFlexPanelElement(child))
                it.main = row ? FlexContentWidth(child, 0, gapMain)
                              : (std::max)(20, xmlNaturalHeight(child));
            else
                it.main = row ? NaturalWidth(tag) : DefaultHeight(tag);

            if (hasCross)
                it.cross = row ? child.h : child.w;
            else
                it.cross = (std::max)(0, crossSize - it.cStart - it.cEnd);   // stretch
            // CSS aspect-ratio：flex 子项同样生效 —— HTML 里图片/封面卡片几乎都在 div（flex）里。
            // 只写一边时另一边按比例算，优先级高于 align-items:stretch。
            bool aspectCross = false;
            {
                const std::wstring ar = child.get(L"AspectRatio");
                float ratio = 0.0f;
                if (!ar.empty() && _wcsicmp(ar.c_str(), L"auto") != 0)
                {
                    const std::wstring::size_type slash = ar.find(L'/');
                    if (slash != std::wstring::npos)
                    {
                        const float a = static_cast<float>(_wtof(ar.substr(0, slash).c_str()));
                        const float b = static_cast<float>(_wtof(ar.substr(slash + 1).c_str()));
                        if (a > 0.0f && b > 0.0f)
                            ratio = a / b;
                    }
                    else
                    {
                        const float v = static_cast<float>(_wtof(ar.c_str()));
                        if (v > 0.0f)
                            ratio = v;
                    }
                }
                if (ratio > 0.0f)
                {
                    // 用属性原串判"作者写死"：块级容器被铺满/autoH 时 h 会有默认值，
                    // 但那不算声明，比例仍应接管（与普通流分支同一判据）。
                    auto declLen2 = [](const std::wstring& s) {
                        if (s.empty() || _wcsicmp(s.c_str(), L"auto") == 0)
                            return false;
                        if (s.find(L'%') != std::wstring::npos && _wtof(s.c_str()) >= 100.0f)
                            return false;
                        return true;
                    };
                    const bool awDecl = declLen2(child.get(L"Width"));
                    const bool ahDecl = declLen2(child.get(L"Height"));
                    int aw = awDecl ? child.w : 0;
                    int ah = ahDecl ? child.h : 0;
                    // HTML 语义：aspect-ratio 只要有一边定下来，另一边就按比例推出来，
                    // 不管那一侧作者有没有写（只写 height 的封面卡片也要得到正确宽度）。
                    if (aw > 0 && ah <= 0)
                        ah = (std::max)(1, static_cast<int>(aw / ratio + 0.5f));
                    else if (ah > 0 && aw <= 0)
                        aw = (std::max)(1, static_cast<int>(ah * ratio + 0.5f));
                    if (row)
                    {
                        if (aw > 0) it.main = aw;
                        if (ah > 0) { it.cross = ah; aspectCross = !ahDecl; }
                    }
                    else
                    {
                        if (ah > 0) it.main = ah;
                        if (aw > 0) { it.cross = aw; aspectCross = !awDecl; }
                    }
                }
            }
            if (it.main < 1) it.main = 1;
            if (it.cross < 1) it.cross = 1;
            items.push_back(it);

            // 控件层参数：自然尺寸与"是否声明"要和上面一致，交叉轴没声明就别把 stretch 值当自然值
            XmlFlexItem pi;
            // 子项真实控件 id：容器自己重排时要靠它 moveControl（没有 id 就永远排不动）
            if (child.control != nullptr)
                pi.id = static_cast<XmlMfcControlBase*>(child.control)->Id();
            pi.naturalW = row ? it.main : (hasCross || aspectCross ? it.cross
                                           : (std::max)(20, DefaultHeight(tag)));
            pi.naturalH = row ? (hasCross || aspectCross ? it.cross
                                    : (std::max)(20, DefaultHeight(tag)))
                              : it.main;
            pi.basis = child.flexBasis >= 0 ? child.flexBasis : it.main;
            // style 里的百分比按容器内容盒换算（row 主轴是宽、column 主轴是高）。
            // flex-basis 自己写的百分比优先于 width/height 的百分比（CSS 里 basis 盖过 width）。
            pi.basisPct = child.flexBasisPct >= 0.0f ? child.flexBasisPct
                                                   : (row ? child.pctW : child.pctH);
            pi.crossPct = row ? child.pctH : child.pctW;
            // 交叉轴尺寸：autoH（HTML 块容器靠内容撑高）对 flex 来说等于"没写高度"，
            // 必须交 -1 给容器，align-items:stretch 才会把它拉满行高。
            bool crossIsAuto = row ? child.autoH : false;
            if (!row)
            {
                // column 容器里交叉轴是宽度：只要作者没写 width，就交给容器按 align-items 处理。
                // HTML 里 <div style="flex-direction:column"> 的子项默认 align-items:stretch 拉满整行宽，
                // 写不写 width 都不会退回"内容宽"（否则卡片标题会被裁成"图纸"而不是"图纸解析"）。
                // button/input 的 88/160 是"HTML 默认尺寸表"兜底值，同样不算作者声明。
                if (child.get(L"Width").empty())
                    crossIsAuto = true;
            }
            if (aspectCross)
                crossIsAuto = false;   // 比例算出来的交叉轴就是确定的，不能再被 stretch 改写
            pi.crossSize = (hasCross || aspectCross) && !crossIsAuto ? it.cross : -1;
            pi.grow = it.grow;
            pi.shrink = child.flexShrink;
            pi.selfAlign = it.self;
            pi.marginL = child.marginL;
            pi.marginT = child.marginT;
            pi.marginR = child.marginR;
            pi.marginB = child.marginB;
            pi.marginLAuto = child.marginLAuto;
            pi.marginTAuto = child.marginTAuto;
            pi.marginRAuto = child.marginRAuto;
            pi.marginBAuto = child.marginBAuto;
            // CSS 尺寸上下限：min-width/max-width/min-height/max-height
            // （row 主轴是宽，column 主轴是高；容器按主轴/交叉轴投影）
            auto sizeAttr = [&](const wchar_t* a, const wchar_t* b) -> int {
                const std::wstring v1 = child.get(b);
                const std::wstring v2 = child.get(a);
                const std::wstring v = !v1.empty() ? v1 : v2;
                return v.empty() ? -1 : (int)wcstol(v.c_str(), nullptr, 10);
            };
            pi.minW = sizeAttr(L"min-width", L"MinWidth");
            pi.maxW = sizeAttr(L"max-width", L"MaxWidth");
            pi.minH = sizeAttr(L"min-height", L"MinHeight");
            pi.maxH = sizeAttr(L"max-height", L"MaxHeight");
            // align-items:baseline 用的「项顶到文字基线」距离（复用既有 BaselineFromTop，
            // 它已按字号 ascent/descent 与垂直居中算好）。CSS 里 baseline 只对横排有意义。
            if (row)
            {
                const std::wstring k2 = KindOf(child.tag);
                const bool texty = k2 == L"label" || k2 == L"span" || k2 == L"p" ||
                                   k2 == L"button" || k2 == L"edit" || k2 == L"text";
                if (texty)
                    pi.baseline = BaselineFromTop(child, it.cross);
            }
            panelItems.push_back(pi);
        }

        // 控件层 flex：容器控件自己算矩形（wrap / align-self / flex-basis / 行间距都在控件里）
        if (flexPanelTag && !panelItems.empty())
        {
            // 把子项清单交给容器本体：容器自己持有子项、自己算子项矩形、自己 moveControl。
            // 这是"控件层 flex"的关键一步 —— 缺了它容器的 m_items 恒为空，
            // 容器 setBounds/WM_SIZE 时无事可做，子项尺寸永远停在创建时那一次的结果
            // （表现为卡片标题被内容宽裁掉、正文互相串行）。
            // P0：真实容器实例在就让它当唯一决策者 —— WM_SIZE 重排用的正是这个实例的
            // m_items，XmlCore 再另开一份栈上实例就会出现"两套参数、两套结果"。
            // 控件还没创建时（layout 早于 build）才回退到栈上临时实例。
            XmlFlexPanel* impl = &panelImpl;
            if (node.control != nullptr)
            {
                if (auto* real = dynamic_cast<XmlFlexPanel*>(
                        static_cast<XmlMfcControlBase*>(node.control)))
                    impl = real;
            }
            impl->clearItems();
            // 属性也要同步到真实实例（幂等）：layout 可能早于 AttachPanel，
            // 实例上还留着默认值（nowrap / gap=0 / justify=start），不灌就会排错。
            if (impl != &panelImpl)
            {
                if (KindOf(node.tag) == L"column")
                    impl->setDirection(XmlFlexDir::Column);
                for (const auto& kv : node.attrs)
                    impl->setOwnProperty(LocalName(kv.first).c_str(), kv.second);
                if (impl->rowGap() <= 0 && impl->columnGap() <= 0 && node.spacing > 0)
                    impl->setGap(node.spacing, node.spacing);
            }
            for (const XmlFlexItem& p : panelItems)
                impl->addItem(p);
            // 容器自己扣 padding：这里必须把「整块外框」交给它，不能先扣一遍，
            // 否则子项会被 padding 偏移两次（HTML 版 div 的 padding 叠加 bug 就在这）
            const int frameW = flow.width;
            // auto 高度：row 方向且作者没写 Height 时，容器高 = 各行高 + row-gap。
            // 累计口径由控件层 measureDesiredCross 回报（换行/行高的唯一决策者在控件层），
            // XmlCore 只把结果写回节点几何；显式 Height / 百分比高完全不走这条，固定尺寸不被撑大。
            int frameH = (flow.height > 0 ? flow.height : 0);
            if (row && !node.hasH)
            {
                const int desired = impl->measureDesiredCross(flow.width, frameH, panelItems) +
                                    node.padT + node.padB;
                if (desired > 0)
                {
                    node.h = desired;
                    node.autoH = true;
                    node.hasH = true;
                    node.relH = desired;
                    node.previewH = desired;
                    frameH = desired;
                }
            }
            // auto 宽（column + wrap）：写死可用高、作者没写 Width 时，容器按拆列结果撑开
            // 宽度 = 各列宽之和 + column-gap；没有显式高控件层不拆列（返回 0），退回原有兜底。
            // 显式 Width 一律以作者为准，不走这条。
            if (!row && node.hasH && !node.hasW)
            {
                const int desiredW = impl->measureDesiredCross(flow.width, frameH, panelItems) +
                                     node.padL + node.padR;
                if (desiredW > 0)
                {
                    node.w = desiredW;
                    node.autoW = true;
                    node.hasW = true;
                    node.relW = desiredW;
                    node.previewW = desiredW;
                }
            }
            const std::vector<XmlFlexRect> rects = impl->computeLayout(frameW, frameH, panelItems);
            // P0 规则：flex 子项矩形只有一个决策者 —— 上面的控件层算法（真实容器跑同一份
            // ComputeLayout）。XmlCore 这里只做「相对坐标」产出：relX/relY/relW/relH 是相对
            // 容器客户区的值，绝对坐标 = 容器原点 + 相对值，拼接点只有这一处。
            const size_t count = (std::min)(rects.size(), items.size());
            for (size_t i = 0; i < count; ++i)
            {
                const XmlFlexRect& r = rects[i];
                XmlElement& cel = *items[i].el;
                cel.relX = r.x;
                cel.relY = r.y;
                cel.relW = r.w;
                cel.relH = r.h;
                FlexLayoutChild(cel, flow.offsetX + r.x, flow.offsetY + r.y, r.w, r.h);
            }
            // 真实容器实例在时，把这一版矩形直接落到子窗口：适配层只递坐标，
            // 容器自己 moveControl（否则加载阶段算出来的 rect 只改元素几何，
            // 窗口仍停在创建时的尺寸，出现"元素 160 宽、控件 428 宽"的错位）。
            if (impl != &panelImpl)
                impl->applyRects(rects);
            if (!freeOnes.empty())
            {
                for (XmlElement* a : freeOnes)
                    layoutFree(a);
            }
            return;
        }

        const int n = static_cast<int>(items.size());
        int used = gapMain * (n > 0 ? n - 1 : 0);
        float totalGrow = 0.0f;
        for (const FlexItem& it : items)
        {
            used += it.main + it.mStart + it.mEnd;
            if (it.grow > 0.0f) totalGrow += it.grow;
        }
        int freeMain = mainSize - used;
        if (freeMain > 0 && totalGrow > 0.0f)
        {
            int given = 0, lastGrow = -1;
            for (int i = 0; i < n; ++i)
            {
                if (items[(size_t)i].grow <= 0.0f)
                    continue;
                const int add = static_cast<int>(freeMain * items[(size_t)i].grow / totalGrow);
                items[(size_t)i].main += add;
                given += add;
                lastGrow = i;
            }
            // 整除余数补给最后一个 grow 项，避免"差 1px 铺不满"
            if (lastGrow >= 0)
            {
                items[(size_t)lastGrow].main += freeMain - given;
                given = freeMain;
            }
            freeMain -= given;
        }
        if (freeMain < 0)
            freeMain = 0;

        // CSS margin:auto —— 主轴上的 auto 边距先均分掉剩余空间，剩下的才轮到
        // justify-content（跟控件层容器同一套规则，两条排版路径保持一致）。
        int autoCount2 = 0;
        for (const FlexItem& it : items)
        {
            if (it.autoStart)
                ++autoCount2;
            if (it.autoEnd)
                ++autoCount2;
        }
        if (autoCount2 > 0 && freeMain > 0)
        {
            const int each = freeMain / autoCount2;
            for (FlexItem& it : items)
            {
                if (it.autoStart)
                    it.mStart += each;
                if (it.autoEnd)
                    it.mEnd += each;
            }
            freeMain = 0;
        }

        int lead = 0, extra = 0;
        switch (node.flexJustify)
        {
        case 1: lead = freeMain / 2; break;
        case 2: lead = freeMain; break;
        case 3: extra = n > 1 ? freeMain / (n - 1) : 0; break;
        case 4: extra = n > 0 ? freeMain / n : 0; lead = extra / 2; break;
        case 5: extra = freeMain / (n + 1); lead = extra; break;
        default: break;
        }

        int cursor = lead;
        for (FlexItem& it : items)
        {
            const int mainPos = cursor + it.mStart;
            const int pos = reverse ? (mainSize - (mainPos + it.main)) : mainPos;
            const int selfAlign = it.self >= 0 ? it.self : node.flexAlign;
            int crossBox = it.cross;
            int crossPos = it.cStart;
            // CSS：交叉轴 auto 边距优先于 align-items，剩余交叉空间均分给 auto 边
            if (it.autoCStart || it.autoCEnd)
            {
                int freeCross = crossSize - (crossBox + it.cStart + it.cEnd);
                if (freeCross > 0)
                {
                    int cnt = (it.autoCStart ? 1 : 0) + (it.autoCEnd ? 1 : 0);
                    const int each = freeCross / cnt;
                    if (it.autoCStart)
                        it.cStart += each;
                    if (it.autoCEnd)
                        it.cEnd += each;
                }
                crossPos = it.cStart;
            }
            else if (selfAlign == 3)
                crossBox = (std::max)(1, crossSize - it.cStart - it.cEnd);
            else if (selfAlign == 1)
                crossPos = (crossSize - it.cStart - it.cEnd - crossBox) / 2 + it.cStart;
            else if (selfAlign == 2)
                crossPos = crossSize - it.cEnd - crossBox;
            if (crossBox < 1) crossBox = 1;
            if (crossPos < 0) crossPos = 0;

            const int px = cx + (row ? pos : crossPos);
            const int py = cy + (row ? crossPos : pos);
            const int pw = (std::max)(1, row ? it.main : crossBox);
            const int ph = (std::max)(1, row ? crossBox : it.main);
            FlexLayoutChild(*it.el, px, py, pw, ph);
            cursor = mainPos + it.main + it.mEnd + gapMain + extra;
        }

        if (!freeOnes.empty())
        {
            for (XmlElement* a : freeOnes)
                layoutFree(a);
        }
    }

    // 落位一个 flex 子项并继续它的子布局（grid 轨道在这里一并解析）
    void FlexLayoutChild(XmlElement& child, int px, int py, int pw, int ph)
    {
        child.previewX = px;
        child.previewY = py;
        child.previewW = pw;
        child.previewH = ph;
        XmlFlow inner;
        inner.margin = 0;
        inner.spacing = child.spacing;
        inner.padding = child.padL;
        inner.paddingY = child.padT;
        inner.width = pw;
        inner.height = ph;
        inner.offsetX = px;
        inner.offsetY = py;
        inner.cursorX = child.padL;
        inner.cursorY = child.padT;
        inner.horizontal = child.horizontal;
        // 轨道可用空间按 CSS 的 content box：扣掉容器自己的 padding
        if (!child.gridCols.empty())
            inner.gridCols = ResolveTracks(child.gridCols, false, child.children,
                                           (std::max)(1, pw - child.padL - child.padR));
        if (!child.gridRows.empty())
            inner.gridRows = ResolveTracks(child.gridRows, true, child.children,
                                           (std::max)(1, ph - child.padT - child.padB));
        LayoutNode(child, inner);
    }

    void LayoutNode(XmlElement& node, XmlFlow& flow)
    {
        // flex 容器：整层交给 flex 引擎，不再走流式/绝对混排
        // 控件层容器（XmlFlexPanel / FlexBox）本身就是一个 flex 容器，不写字也是 flex（HTML 的 div 即块容器）
        if (node.flex || IsFlexPanelElement(node))
        {
            LayoutFlex(node, flow);
            return;
        }
        // ItemView 行定位：delegate 行按 ItemHeight 逐行堆叠，行内再走普通布局
        if (_wcsicmp(KindOf(node.tag).c_str(), L"itemview") == 0)
        {
            int rowH = _wtoi(node.get(L"ItemHeight").c_str());
            if (rowH <= 0)
                rowH = 44;
            const int availW = (std::max)(1, flow.width - flow.padding * 2);
            int i = 0;
            for (auto& child : node.children)
            {
                child.previewX = flow.offsetX + flow.padding;
                child.previewY = flow.offsetY + flow.paddingY + i * rowH;
                child.previewW = availW;
                child.previewH = rowH;

                XmlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = 0;
                inner.paddingY = 0;
                inner.width = availW;
                inner.height = rowH;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                inner.cursorX = 0;
                inner.cursorY = 0;
                inner.horizontal = child.hasOrientation ? child.horizontal : true;   // 行内默认横排
                LayoutNode(child, inner);
                ++i;
            }
            return;
        }
        // 基线对齐（Qt.AlignBaseline）用：本层子项里最大的"盒顶到基线"距离
        int rowMaxAsc = 0;
        for (const auto& c : node.children)
        {
            const std::wstring ct = KindOf(c.tag);
            if (ct == L"page" || ct == L"rowdefinition" || ct == L"columndefinition")
                continue;
            rowMaxAsc = (std::max)(rowMaxAsc, BaselineFromTop(c));
        }

        const bool paletteRoot = KindOf(node.tag) == L"palette";
        // grid 的 auto-placement 游标（HTML 默认 grid-auto-flow:row）
        int gridAcCol = 0, gridAcRow = 0;
        bool firstPage = true;

        for (auto& child : node.children)
        {
            const std::wstring tag = KindOf(child.tag);
            // 侧栏的多页：预览与设计器只画第一页（其余页数据保留，运行时靠 tab 切换）
            if (tag == L"page")
            {
                if (paletteRoot && !firstPage)
                    continue;
                firstPage = false;
            }
            else if (tag == L"rowdefinition" || tag == L"columndefinition")
                continue;

            // 容器没写 Height：横排容器按内容给高（横排不该纵向撑满父容器），
            // 竖排容器保持原有"占满可用高"的语义（既有布局依赖它）。
            const int defH = IsContainer(tag)
                ? ((child.horizontal && !child.hasH)
                       ? xmlNaturalHeight(child)
                       : (flow.height > 0 ? flow.height : 160))
                : DefaultHeight(tag);
            int h = child.hasH ? child.h : defH;
            // CSS 百分比尺寸按父内框换算。此前只有 flex 容器的子项读 pctW/pctH，
            // 普通块级元素写 width="50%" 会被当成"没写宽度"直接撑满 —— 与浏览器差一整个语义。
            if (child.pctH > 0.0f && child.pctH < 100.0f)
            {
                const int baseH = flow.height > 0 ? flow.height - flow.paddingY * 2 : defH;
                h = (std::max)(1, baseH * static_cast<int>(child.pctH + 0.5f) / 100);
            }
            // 没写宽：竖排 = 撑满一行；横排 = 用控件自然宽（HTML 行内元素的行为）
            int w = 0;
            if (child.pctW > 0.0f && child.pctW < 100.0f)
            {
                const int baseW = (std::max)(1, flow.width - flow.padding * 2);
                w = (std::max)(1, baseW * static_cast<int>(child.pctW + 0.5f) / 100);
            }
            else if (child.hasW)
                w = child.w;
            else if (flow.horizontal)
                w = NaturalWidth(tag);
            else
                w = (std::max)(1, flow.width - flow.margin * 2 - flow.padding * 2);

            // CSS 绝对定位的另一半：同时写了 left+right（或 top+bottom）、又没写尺寸时，
            // 尺寸由两条边反推 —— 这正是 `inset:12 20` 的语义（等价于撑满父再留边）。
            // 早先 Right/Bottom 落成属性后布局根本不读，作者写了 inset 却只生效 Top/Left，
            // 尺寸退回默认值，和 HTML 差一整个语义。
            {
                const std::wstring eL = child.get(L"Left"), eR = child.get(L"Right");
                const std::wstring eT = child.get(L"Top"), eB = child.get(L"Bottom");
                // autoTextW / autoH 是 HTML 糖化按"内容"测出来的自动尺寸，
                // 作者没写 Width/Height；一旦左右（上下）两条边都写了，CSS 的算法就是
                // "由两条边反推尺寸"，自动内容尺寸必须让位。
                if (!eL.empty() && !eR.empty() && (!child.hasW || child.autoTextW))
                {
                    const int base = (std::max)(1, flow.width);
                    w = (std::max)(1, base - CssEdgePx(eL, base, 0) - CssEdgePx(eR, base, 0));
                }
                if (!eT.empty() && !eB.empty() && (!child.hasH || child.autoH) && flow.height > 0)
                {
                    const int base = (std::max)(1, flow.height);
                    h = (std::max)(1, base - CssEdgePx(eT, base, 0) - CssEdgePx(eB, base, 0));
                }
                // `inset:0 10%` 这类百分比左边距：属性解析走的是 _wtoi，会读成 10px。
                // 这里归一成像素并标记为已定位，免得后面按流式摆放。
                if (!eL.empty() && eL.find(L'%') != std::wstring::npos)
                {
                    child.x = CssEdgePx(eL, (std::max)(1, flow.width), 0);
                    child.hasX = true;
                }
                if (!eT.empty() && eT.find(L'%') != std::wstring::npos && flow.height > 0)
                {
                    child.y = CssEdgePx(eT, (std::max)(1, flow.height), 0);
                    child.hasY = true;
                }
            }

            // 坐标：默认流式，写全 Left/Top 用绝对
            const bool useGridX = !flow.gridCols.empty();
            const bool useGridY = !flow.gridRows.empty();
            int localX = 0, localY = 0;

            // inset:0：占满父内框（fillMargin 为四边内缩），不参与流式推进
            bool anchored = false;
            if (child.fillParent)
            {
                const int availL = flow.padding;
                const int availT = flow.paddingY;
                const int availR = (std::max)(availL + 1, flow.width - flow.padding);
                const int availB = (std::max)(availT + 1, flow.height - flow.paddingY);
                const int m = child.fillMargin;
                localX = availL + m;
                localY = availT + m;
                w = (std::max)(1, availR - availL - m * 2);
                h = (std::max)(1, availB - availT - m * 2);
                anchored = true;
            }

            if (!anchored)
            {
            if (flow.horizontal)
            {
                localX = child.hasX ? child.x : flow.cursorX;
                localY = child.hasY ? child.y : flow.cursorY;
                if (!useGridX && !useGridY)
                    flow.cursorX = (std::max)(flow.cursorX, localX + w + flow.spacing);
            }
            else
            {
                localX = child.hasX ? child.x : flow.padding;
                localY = child.hasY ? child.y : flow.cursorY;
                if (!useGridX && !useGridY)
                {
                    if (child.hasY)
                        flow.cursorY = (std::max)(flow.cursorY, localY + h + flow.spacing);
                    else
                        flow.cursorY += h + flow.spacing;
                }
            }

            // CSS 只写 right/bottom（没写 left/top）：贴父内框的右/下边。
            // HTML 里 `style="right:20px;bottom:12px"` 就是右下角定位，宽高仍按内容。
            {
                const std::wstring onlyR = child.get(L"Right");
                const std::wstring onlyB = child.get(L"Bottom");
                if (!child.hasX && !onlyR.empty())
                {
                    const int r = CssEdgePx(onlyR, flow.width, 0);
                    localX = (std::max)(flow.padding, flow.width - flow.padding - r - w);
                }
                if (!child.hasY && !onlyB.empty() && flow.height > 0)
                {
                    const int b = CssEdgePx(onlyB, flow.height, 0);
                    localY = (std::max)(flow.paddingY, flow.height - flow.paddingY - b - h);
                }
            }

            // Grid 单元格：行、列两个维度各自独立生效，未声明的维度退回 Left/Top
            if (useGridX || useGridY)
            {
                if (useGridX && !useGridY && !child.hasY)
                    localY = 0;   // 只有列定义时，子元素都在同一行
                if (useGridY && !useGridX && !child.hasX)
                    localX = 0;
                if (useGridX)
                {
                    const int col = (std::max)(0, (std::min)(
                        child.hasGridCol ? child.col : gridAcCol, (int)flow.gridCols.size() - 1));
                    const int colSpan =
                        (std::max)(1, (std::min)(child.colSpan, (int)flow.gridCols.size() - col));
                    localX = flow.padding + Sum(flow.gridCols, 0, col);
                    const int cellW = Sum(flow.gridCols, col, colSpan);
                    // CSS Grid 的项默认拉伸到单元格：按内容测出来的伪宽度（autoTextW）
                    // 不能当作"作者写了宽度"，否则格子里只剩文字宽。
                    if (child.hasW && !child.autoW && !child.autoTextW && !child.sugarW)
                        w = (std::min)(w, cellW);
                    else
                        w = cellW;
                }

                if (useGridY)
                {
                    const int row = (std::max)(0, (std::min)(
                        child.hasGridRow ? child.row : gridAcRow, (int)flow.gridRows.size() - 1));
                    const int rowSpan =
                        (std::max)(1, (std::min)(child.rowSpan, (int)flow.gridRows.size() - row));
                    localY = flow.paddingY + Sum(flow.gridRows, 0, row);
                    const int cellH = Sum(flow.gridRows, row, rowSpan);
                    // 内容高度（autoH）同样不能算作"作者写了高度"，否则格子里只剩自然高
                    if (child.hasH && !child.autoH && !child.sugarH)
                        h = (std::min)(h, cellH);
                    else
                        h = cellH;
                }
                // CSS auto-placement：没显式落格时按源顺序顺次填格（行优先）
                if (useGridX && !child.hasGridCol)
                {
                    gridAcCol += (std::max)(1, child.colSpan);
                    if (gridAcCol >= (int)flow.gridCols.size())
                    {
                        gridAcCol = 0;
                        if (useGridY)
                            ++gridAcRow;
                    }
                }
            }
            }   // if (!anchored)

            // Margin（撑满父的项已在 fillMargin 里处理内缩，不再叠加）
            if (!anchored)
            {
                localX += child.marginL;
                localY += child.marginT;
                w = (std::max)(1, w - child.marginL - child.marginR);
                h = (std::max)(1, h - child.marginT - child.marginB);
            }

            // Layout.alignment 落位：垂直流看水平分量，水平流看垂直分量
            if (!anchored)
            {
                const int availW = (std::max)(1, flow.width - flow.padding * 2);
                const int availH = (std::max)(1, flow.height - flow.paddingY * 2);
                if (!flow.horizontal && child.alignX != 0)
                {
                    if (child.alignX == 3)
                    {
                        w = (std::max)(1, availW - child.marginL - child.marginR);
                        localX = flow.padding + child.marginL;
                    }
                    else if (child.alignX == 1)
                        localX = (std::max)(flow.padding, flow.width - flow.padding - w - child.marginR);
                    else
                        localX = flow.padding + (std::max)(0, availW - w) / 2;
                }
                else if (!flow.horizontal && (child.marginLAuto || child.marginRAuto))
                {
                    // CSS `margin: 0 auto` 在块级（纵向流）里的语义：左右 auto 均分剩余空间 → 水平居中。
                    // HTML 里这是最省事的一句居中；此前它只在 flex 容器内生效，
                    // 普通 <div><label style="margin:0 auto"> 会被当成左对齐。
                    const int boxW = (std::max)(1, w);
                    if (child.marginLAuto && child.marginRAuto)
                        localX = flow.padding + (std::max)(0, availW - boxW) / 2;
                    else if (child.marginLAuto)
                        localX = (std::max)(flow.padding, flow.width - flow.padding - boxW);
                    else
                        localX = flow.padding;
                }
                else if (flow.horizontal && child.alignY != 0)
                {
                    if (child.alignY == 4)
                    {
                        // 基线对齐：让本项文字基线与行内基线（最大 ascent 那条）齐平
                        localY = flow.paddingY +
                                 (std::max)(0, rowMaxAsc - BaselineFromTop(child, h));
                    }
                    else if (child.alignY == 3)
                    {
                        h = (std::max)(1, availH - child.marginT - child.marginB);
                        localY = flow.paddingY + child.marginT;
                    }
                    else if (child.alignY == 1)
                        localY = (std::max)(flow.paddingY, flow.height - flow.paddingY - h - child.marginB);
                    else
                        localY = flow.paddingY + (std::max)(0, availH - h) / 2;
                }
            }

            // CSS aspect-ratio：只写一条边 + 比例，另一条边自动算出来。
            // HTML 里图片/视频/封面卡片的标准写法（`width:160px;aspect-ratio:16/9`），
            // XML 早先必须自己算好 height 写死，容器宽度变了比例就崩。
            // 只补"未声明的那一边"，两边都写了就以显式声明为准（与 CSS 一致）。
            {
                const std::wstring ar = child.get(L"AspectRatio");
                if (!ar.empty() && _wcsicmp(ar.c_str(), L"auto") != 0)
                {
                    float ratio = 0.0f;
                    const std::wstring::size_type slash = ar.find(L'/');
                    if (slash != std::wstring::npos)
                    {
                        const float a = static_cast<float>(_wtof(ar.substr(0, slash).c_str()));
                        const float b = static_cast<float>(_wtof(ar.substr(slash + 1).c_str()));
                        if (a > 0.0f && b > 0.0f)
                            ratio = a / b;
                    }
                    else
                    {
                        const float v = static_cast<float>(_wtof(ar.c_str()));
                        if (v > 0.0f)
                            ratio = v;
                    }
                    if (ratio > 0.0f)
                    {
                        // 用"属性原串"判显式声明：块级元素被流布局铺满时 w 会变大，
                        // 但 Width 属性仍是空的 —— 那属于"没写宽"，比例可以接管。
                        auto declLen = [](const std::wstring& s) {
                            if (s.empty() || _wcsicmp(s.c_str(), L"auto") == 0)
                                return false;
                            if (s.find(L'%') != std::wstring::npos &&
                                _wtof(s.c_str()) >= 100.0f)
                                return false;      // 100% = 铺满，等于"没写死"
                            return true;
                        };
                        const bool declW = declLen(child.get(L"Width"));
                        const bool declH = declLen(child.get(L"Height"));
                        if (!declH && declW)
                            h = (std::max)(1, static_cast<int>(w / ratio + 0.5f));
                        else if (!declW && declH)
                            w = (std::max)(1, static_cast<int>(h * ratio + 0.5f));
                        else if (!declH && !declW && w > 0)
                            h = (std::max)(1, static_cast<int>(w / ratio + 0.5f));
                    }
                }
            }

            // CSS min-width / max-width：块级流与绝对定位元素同样遵守（HTML 里这跟 flex 无关）。
            // 早先只有 flex 子项读 MinWidth/MaxWidth，普通 <div style="width:300;max-width:150">
            // 仍然画 300 —— "最多 300、窄屏不许低于 120" 这种最常见的写法在 XML 里失效。
            // 放在落位之后：先按 width/百分比/反推/网格算出来，再用上下限夹一次。
            {
                const std::wstring mnS = !child.get(L"min-width").empty()
                                             ? child.get(L"min-width")
                                             : child.get(L"MinWidth");
                const std::wstring mxS = !child.get(L"max-width").empty()
                                             ? child.get(L"max-width")
                                             : child.get(L"MaxWidth");
                const int baseW = (std::max)(1, flow.width - flow.padding * 2);
                if (!mnS.empty() && _wcsicmp(mnS.c_str(), L"auto") != 0)
                {
                    const int mn = CssEdgePx(mnS, baseW, -1);
                    if (mn > 0 && w < mn)
                        w = mn;
                }
                if (!mxS.empty() && _wcsicmp(mxS.c_str(), L"none") != 0)
                {
                    const int mx = CssEdgePx(mxS, baseW, -1);
                    if (mx > 0 && w > mx)
                        w = mx;
                }
                // 高度同口径（HTML 里 max-height 定"最高多少"、min-height 定"最矮多少"）
                const std::wstring mnHS = !child.get(L"min-height").empty()
                                              ? child.get(L"min-height")
                                              : child.get(L"MinHeight");
                const std::wstring mxHS = !child.get(L"max-height").empty()
                                              ? child.get(L"max-height")
                                              : child.get(L"MaxHeight");
                const int baseH = (std::max)(1, flow.height - flow.paddingY * 2);
                if (!mnHS.empty() && _wcsicmp(mnHS.c_str(), L"auto") != 0)
                {
                    const int mn = CssEdgePx(mnHS, baseH, -1);
                    if (mn > 0 && h < mn)
                        h = mn;
                }
                if (!mxHS.empty() && _wcsicmp(mxHS.c_str(), L"none") != 0)
                {
                    const int mx = CssEdgePx(mxHS, baseH, -1);
                    if (mx > 0 && h > mx)
                        h = mx;
                }
            }

            // <img> 被宽度/高度任一维夹住时按原图比例缩另一维（HTML 的"替换元素保留
            // 宽高比"行为）。NaturalWidth/NaturalHeight 由加载层按文件头写入。
            // 三种情形：①只夹宽 → 按宽缩高（max-width:100% 的响应式图片）；
            // ②只夹高 → 按高缩宽（max-height:120 常见于列表里的缩略图）；
            // ③两维都夹 → 取两个缩放比的较小值（等于 contain，浏览器同口径），
            //   否则图会被硬拉成畸形比例。普通控件不走这段：显式高就是显式高。
            if (::_wcsicmp(child.tag.c_str(), L"XImage") == 0)
            {
                const int natW = static_cast<int>(_wtoi(child.get(L"NaturalWidth").c_str()));
                const int natH = static_cast<int>(_wtoi(child.get(L"NaturalHeight").c_str()));
                if (natW > 0 && natH > 0)
                {
                    auto shrinkBy = [](int other, int nat, int base) {
                        const int v = static_cast<int>(
                            static_cast<double>(nat) * other / base + 0.5);
                        return (std::max)(1, v);
                    };
                    if (w != natW && h == natH)
                        h = shrinkBy(w, natH, natW);
                    else if (h != natH && w == natW)
                        w = shrinkBy(h, natW, natH);
                    else if (w != natW && h != natH)
                    {
                        const double sx = static_cast<double>(w) / natW;
                        const double sy = static_cast<double>(h) / natH;
                        const double s = (std::min)(sx, sy);
                        w = (std::max)(1, static_cast<int>(natW * s + 0.5));
                        h = (std::max)(1, static_cast<int>(natH * s + 0.5));
                    }
                }
            }

            child.previewX = flow.offsetX + localX;
            child.previewY = flow.offsetY + localY;
            child.previewW = w;
            child.previewH = h;
            // 布局诊断（PYARX_XML_LAYOUT_DBG=1 时把每一项的落位写进 log\xml_layout.log）
            if (g_layoutDbg)
            {
                FILE* f = nullptr;
                static std::wstring dbgPath;
                if (dbgPath.empty())
                    dbgPath = xmlprop2::LogDir() + L"\\xml_layout.log";
                if (_wfopen_s(&f, dbgPath.c_str(), L"a") == 0 && f != nullptr)
                {
                    fprintf(f, "  %s y=%d h=%d cursorY=%d spacing=%d flowH=%d horiz=%d\n",
                            xmlprop::WideToUtf8(child.tag).c_str(), localY, h, flow.cursorY,
                            flow.spacing, flow.height, flow.horizontal ? 1 : 0);
                    fclose(f);
                }
            }

            // 容器递归
            if (tag == L"stackpanel" || tag == L"page" || tag == L"palette")
            {
                XmlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = child.padL;
                inner.paddingY = child.padT;
                inner.width = w;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                inner.cursorX = child.padL;
                inner.cursorY = child.padT;
                inner.horizontal = child.horizontal;
                inner.height = h;
                LayoutNode(child, inner);
            }
            else if (tag == L"grid")
            {
                XmlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = child.padL;
                inner.paddingY = child.padT;
                inner.width = w;
                inner.height = h;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                // 只有 XML 显式声明了对应维度的轨道时才生成轨道，未声明则保留 Left/Top 绝对定位
                if (!child.gridCols.empty())
                    inner.gridCols = ResolveTracks(child.gridCols, false, child.children,
                                                   (std::max)(1, w - child.padL - child.padR));
                if (!child.gridRows.empty())
                    inner.gridRows = ResolveTracks(child.gridRows, true, child.children,
                                                   (std::max)(1, h - child.padT - child.padB));
                LayoutNode(child, inner);
            }
            else if (tag == L"dockpanel" || tag == L"wrappanel" || tag == L"canvas" ||
                     tag == L"absolutepanel" || tag == L"abspanel" || tag == L"canvaspanel" ||
                     tag == L"canvaslayout")
            {
                XmlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = child.padL;
                inner.paddingY = child.padT;
                inner.width = w;
                inner.height = h;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                inner.cursorX = child.padL;
                inner.cursorY = child.padT;
                inner.horizontal = (tag == L"wrappanel");
                LayoutNode(child, inner);
            }
            else if (tag == L"toolbar" || tag == L"menubar" || tag == L"statusbar")
            {
                XmlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = child.padL;
                inner.paddingY = child.padT;
                inner.width = w;
                inner.height = h;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                inner.cursorX = child.padL;
                inner.cursorY = child.padT;
                inner.horizontal = child.hasOrientation ? child.horizontal : true;
                LayoutNode(child, inner);
            }
            else if (tag == L"group")
            {
                // group 是控件，也是容器：内部坐标相对组内区，独立流式游标
                // 内区偏移是 GroupBox 边框的物理占位（可经 xmlSetGroupInset 覆盖）
                XmlFlow inner;
                inner.margin = layoutDefaults().groupLeft;
                inner.spacing = child.spacing;
                inner.cursorY = layoutDefaults().groupTop;
                inner.width = (std::max)(w - layoutDefaults().groupLeft * 2, 40);
                inner.offsetX = child.previewX + layoutDefaults().groupLeft;
                inner.offsetY = child.previewY + layoutDefaults().groupTop;
                LayoutNode(child, inner);
            }
            else if (!child.children.empty())
            {
                // 控件模板实体化出来的视觉层：当作该控件内部的容器排布
                XmlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = child.padL;
                inner.paddingY = child.padT;
                inner.width = w;
                inner.height = h;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                inner.cursorX = child.padL;
                inner.cursorY = child.padT;
                inner.horizontal = child.hasOrientation ? child.horizontal : false;
                LayoutNode(child, inner);
            }
        }

        // Layout.fillWidth / Layout.fillHeight 重排：占满可用宽度，或均分剩余高度/宽度
        {
            bool hasFillItem = false;
            for (const auto& c : node.children)
                if (c.fillW || c.fillH)
                {
                    hasFillItem = true;
                    break;
                }

            if (hasFillItem)
            {
                std::vector<XmlElement*> flowItems;
                for (auto& c : node.children)
                    if (!c.fillParent)
                        flowItems.push_back(&c);

                const int availW = (std::max)(1, flow.width - flow.padding * 2);
                const int availH = (std::max)(1, flow.height - flow.paddingY * 2);
                const int gaps = (int)flowItems.size() - 1;
                const int gapTotal = gaps > 0 ? gaps * flow.spacing : 0;

                int fixedMain = 0, nFillMain = 0;
                for (auto* p : flowItems)
                {
                    const bool fillMain = flow.horizontal ? p->fillW : p->fillH;
                    if (fillMain)
                        nFillMain++;
                    else
                        fixedMain += flow.horizontal ? p->previewW : p->previewH;
                }
                const int mainSpace = flow.horizontal ? availW : availH;
                const int rest = mainSpace - fixedMain - gapTotal;
                const int eachMain = nFillMain > 0 ? (std::max)(24, rest / nFillMain) : 0;

                int cursor = flow.horizontal ? flow.padding : flow.paddingY;
                for (auto* p : flowItems)
                {
                    const int ww = flow.horizontal ? (p->fillW ? eachMain : p->previewW) : (p->fillW ? availW : p->previewW);
                    const int hh = flow.horizontal ? (p->fillH ? availH : p->previewH) : (p->fillH ? eachMain : p->previewH);
                    if (flow.horizontal)
                    {
                        p->previewX = flow.offsetX + cursor;
                        p->previewY = flow.offsetY + flow.paddingY;
                    }
                    else
                    {
                        p->previewX = flow.offsetX + flow.padding;
                        p->previewY = flow.offsetY + cursor;
                    }
                    p->previewW = ww;
                    p->previewH = hh;

                    if (!p->children.empty())
                    {
                        XmlFlow inner;
                        inner.margin = 0;
                        inner.spacing = p->spacing;
                        inner.padding = p->padL;
                        inner.paddingY = p->padT;
                        inner.width = ww;
                        inner.height = hh;
                        inner.offsetX = p->previewX;
                        inner.offsetY = p->previewY;
                        inner.cursorX = p->padL;
                        inner.cursorY = p->padT;
                        inner.horizontal = p->hasOrientation ? p->horizontal : false;
                        LayoutNode(*p, inner);
                    }

                    cursor += (flow.horizontal ? ww : hh) + flow.spacing;
                }
            }
        }

    }
}
