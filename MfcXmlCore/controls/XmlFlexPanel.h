#pragma once
// XmlFlexPanel —— 控件层的 flex 容器。
//
// 与 XmlCore 里那套"外层算坐标再 moveControl"的 LayoutFlex 不同：这个控件自己持有
// 子项清单、自己按 CSS flex 语义算子项矩形、自己把子窗口挪到位。宿主只负责给它
// 一个矩形和一份子项清单，重排时机（SetBounds / 子项变化 / 显式请求）都在控件里。
//
// 属性一律用 HTML/CSS 写法（大小写不敏感，中划线/驼峰/无分隔三种都认）：
//   flex-direction  row | row-reverse | column | column-reverse
//   flex-wrap       nowrap | wrap | wrap-reverse
//   justify-content start | center | end | between | around | evenly   (CSS space-between 等也认)
//   align-items     start | center | end | stretch | baseline(按 stretch 处理)
//   gap             单值 或 "行 列"；也可 row-gap / column-gap 分开写
//   padding         单值 或 "左右 上下" 或 "左 上 右 下"（与 XML 属性解析顺序一致）
// 子项属性由容器代为登记：flex-basis / flex-grow / flex-shrink / align-self / margin。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

#include <vector>

// 主轴方向（与 CSS flex-direction 同序）
enum class XmlFlexDir : int
{
    Row = 0,
    RowReverse = 1,
    Column = 2,
    ColumnReverse = 3,
};

// 换行
enum class XmlFlexWrap : int
{
    NoWrap = 0,
    Wrap = 1,
    WrapReverse = 2,
};

// 主轴对齐（justify-content）
enum class XmlFlexJustify : int
{
    Start = 0,
    Center = 1,
    End = 2,
    Between = 3,
    Around = 4,
    Evenly = 5,
};

// 交叉轴对齐（align-items / align-self）
enum class XmlFlexAlign : int
{
    Start = 0,
    Center = 1,
    End = 2,
    Stretch = 3,
    Baseline = 4,
};

// 一个子项：容器排它需要的全部输入
struct XmlFlexItem
{
    int id = 0;
    int naturalW = 0;        // 子项自然宽（未声明尺寸时的回退）
    int naturalH = 0;        // 子项自然高
    int basis = -1;          // flex-basis（主轴尺寸）；-1 = 用自然尺寸
    float basisPct = -1.0f;  // flex-basis 的百分比形式（style="width:50%"）；-1 = 未设
    int crossSize = -1;      // 交叉轴尺寸；-1 = 用自然尺寸
    float crossPct = -1.0f;  // 交叉轴尺寸的百分比形式（style="height:50%"）；-1 = 未设
    float grow = 0.0f;       // flex-grow
    float shrink = 1.0f;     // flex-shrink
    int selfAlign = -1;      // align-self；-1 = 继承容器的 align-items
    int marginL = 0;
    int marginT = 0;
    int marginR = 0;
    int marginB = 0;
    // CSS margin:auto 的标记（数值按 0 记）：主轴上的 auto 边距先吃掉剩余空间，
    // 再轮到 justify-content —— `margin-left:auto` 就是 HTML 里把按钮顶到右端的写法。
    bool marginLAuto = false;
    bool marginTAuto = false;
    bool marginRAuto = false;
    bool marginBAuto = false;
    // CSS 尺寸上下限：-1 = 未设。flex-shrink 不能把子项压到 min 以下，
    // flex-grow / stretch 也不能把子项撑到 max 以上（浏览器正是这么做的）
    int minW = -1;
    int maxW = -1;
    int minH = -1;
    int maxH = -1;
    // CSS align-items:baseline 需要"项顶到文字基线"的距离；-1 = 非文本项（不参与基线对齐）
    int baseline = -1;
    bool absolute = false;   // 已声明显式位置（不参与 flex 流）
};

// 排出来的一个矩形（相对容器左上角，不含容器自身偏移）
struct XmlFlexRect
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

class XML_CORE_API XmlFlexPanel : public XmlMfcControlBase
{
public:
    XmlFlexPanel() : XmlMfcControlBase(L"flexpanel") {}

    // ---- 容器的 flex 声明 ----
    void setDirection(XmlFlexDir d) { m_dir = d; }
    void setWrap(XmlFlexWrap w) { m_wrap = w; }
    void setJustify(XmlFlexJustify j) { m_justify = j; }
    void setAlign(XmlFlexAlign a) { m_align = a; }
    // CSS align-content：多行时行组在交叉轴上的分布（单行容器无意义）
    void setAlignContent(XmlFlexJustify a) { m_alignContent = a; }
    void setAlignContentStretch(bool on) { m_acStretch = on; }
    void setGap(int rowGap, int columnGap) { m_gapY = rowGap; m_gapX = columnGap; }
    void setPadding(int l, int t, int r, int b) { m_padL = l; m_padT = t; m_padR = r; m_padB = b; }

    XmlFlexDir direction() const { return m_dir; }
    XmlFlexWrap wrap() const { return m_wrap; }
    int rowGap() const { return m_gapY; }
    int columnGap() const { return m_gapX; }

    // ---- 子项登记（由适配层在排版前调用）----
    void clearItems() { m_items.clear(); }
    void addItem(const XmlFlexItem& it) { m_items.push_back(it); }
    const std::vector<XmlFlexItem>& items() const { return m_items; }
    // 适配层（XmlCore）算好的一次布局结果落到真实子窗口（absolute 项不参与 flex 流）
    void applyRects(const std::vector<XmlFlexRect>& rects);

    // ---- 重排：控件自己算、自己挪 ----
    // 纯计算（不碰窗口）：给容器尺寸和子项清单，返回每个子项的矩形（相对容器左上角）。
    // 适配层（XmlCore）用它把 XML 属性翻译成参数，再把结果写回元素的 preview。
    std::vector<XmlFlexRect> computeLayout(int frameW, int frameH,
                                           const std::vector<XmlFlexItem>& items) const;

    // 测量（纯计算，不碰窗口）：给定容器外框宽/高，返回 wrap 后交叉轴累计尺寸（不含容器
    // padding）。row 方向 = 各行高之和 + row-gap；column 方向 = 各列宽之和 + column-gap，
    // 且必须有显式可用高（frameH>0）才拆列，否则返回 0 由适配层退回兜底。供 XmlCore 在
    // 容器未写对应维度尺寸时回报 auto 内容尺寸。
    int measureDesiredCross(int frameW, int frameH,
                            const std::vector<XmlFlexItem>& items) const;

    // 只登记不重排的情况用 addItem；清单填完调一次 reflowItems()。
    // reflowItems 走"自己算 + 自己 moveControl"的真实路径（WM_SIZE / 显式请求用）。
    void reflowItems();

    // 覆盖基类：尺寸变化后自己重排子项（这是"控件层 flex"的关键，不依赖宿主整体 reflow）
    void setBounds(int x, int y, int w, int h) override;

    // 加载层据此把子控件 SetParent 到本容器（真实父子层级）
    bool isFlexContainer() const override { return true; }

    // 属性：CSS 写法
    bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;
    std::wstring getOwnProperty(const wchar_t* name) const override;
    std::vector<XmlProperty> ownProperties() const override;

protected:
    void createNative(IXmlMfcHost* host, int id, int minValue, int maxValue,
                      int x, int y, int w, int h) override;

    // 容器背板：交给宿主绘制节点管线（容器自己是原生 HWND，宿主不做 owner-draw，
    // 自绘要走 WM_PAINT；这里统一走节点，圆角/边框/透明度与其它控件同一套实现）
    void pushBackdrop() override;

private:
    // 一行的排版结果
    struct FlexLine
    {
        int first = 0;      // 起始项下标
        int count = 0;      // 本行项数
        int mainUsed = 0;   // 已占主轴
        int crossSize = 0;  // 行高（交叉轴）
    };

    int mainSizeOf(const XmlFlexItem& it, int naturalW, int naturalH) const;
    int crossSizeOf(const XmlFlexItem& it, int naturalW, int naturalH) const;
    int mainMargin(const XmlFlexItem& it) const;
    int crossMargin(const XmlFlexItem& it) const;

    bool m_horizontal = true;                // 主轴是否水平（由 m_dir 推导）
    XmlFlexDir m_dir = XmlFlexDir::Row;
    XmlFlexWrap m_wrap = XmlFlexWrap::NoWrap;
    XmlFlexJustify m_justify = XmlFlexJustify::Start;
    XmlFlexAlign m_align = XmlFlexAlign::Stretch;
    XmlFlexJustify m_alignContent = XmlFlexJustify::Start;
    bool m_acStretch = false;
    int m_gapX = 0;     // 列间距（主轴水平时就是项间距）
    int m_gapY = 0;     // 行间距
    int m_padL = 0;
    int m_padT = 0;
    int m_padR = 0;
    int m_padB = 0;
    std::vector<XmlFlexItem> m_items;
};
