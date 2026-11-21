#pragma once
// PyMfcXml 统一运行时核心（C++，无 MFC 依赖）
//
// 原则：
//   * 所有界面代码由 XML 声明实现；所有接口由 C++ 完成。
//   * 控件通过 XmlControlRegistry 注册（支持自定义控件），不再硬编码 switch。
//   * 属性通过 XmlProperty 元数据反射，设计器属性面板据此动态生成。
//   * IXmlControl 是控件统一接口，宿主（PyMfc / 设计器）提供具体实现与工厂。
//
// 线程安全：运行时核心不持有全局可变状态（注册表为进程级单例，加载/实例化局部）。
// 编码：本文件 UTF-8 with BOM。

#include <windows.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef PYMFCXML_EXPORTS
#define XML_CORE_API __declspec(dllexport)
#else
#define XML_CORE_API __declspec(dllimport)
#endif

// ---------------------------------------------------------------------------
// 属性元数据
// ---------------------------------------------------------------------------
enum class XmlPropType
{
    Int,      // 整数
    String,   // 字符串
    Bool,     // "1"/"0"/"true"/"false"
    Color,    // "#RRGGBB" / "RGB(r,g,b)"
    Enum,     // enumValues 用 "a|b|c"
    Items,    // 条目集（ListBox/ComboBox Items="a|b|c"）
    // 第二批：控件属性表（XmlTypes.h）用到的语义类型。
    // 只决定设计器用什么编辑器、加载期用什么解析，读写仍是字符串。
    Double,     // 实数
    Point,      // "x,y"
    Rect,       // "x,y,w,h"
    Thickness,  // "l,t,r,b" / 单值
    Image,      // 图片路径（相对 exe 目录）
    Columns,    // 列定义 "名称:180|状态:100"
    Rows,       // 行定义 "甲\t1|乙\t2"
    Flags,      // 逗号分隔的开关集 "a,b"
};

struct XmlProperty
{
    const wchar_t* name = nullptr;
    XmlPropType type = XmlPropType::String;
    bool readable = true;
    bool writable = true;
    const wchar_t* enumValues = nullptr; // Enum 类型时用
};

// ---------------------------------------------------------------------------
// XmlElement：统一节点模型（文档树）
// ---------------------------------------------------------------------------
#include <map>

class XmlElement
{
public:
    std::wstring tag;                                   // 标签名（Button/Grid/...）
    std::wstring name;                                  // x:Name / Name
    std::wstring text;                                  // Content/Text/Header
    std::wstring icon;                                  // Icon（res/xxx.png，相对 exe 目录）
    std::vector<std::pair<std::wstring, std::wstring>> attrs; // 全部 XML 属性（有序）
    // 由 Style / 绑定注入的"有效属性"：读取时优先本地 attrs，其次这里；
    // 写回文档时只看 attrs，所以样式不会污染 XML 源码
    std::vector<std::pair<std::wstring, std::wstring>> styledAttrs;
    std::vector<XmlElement> children;                  // 子节点（不含行列定义等元数据）

    // 布局声明值（XML 属性解析结果）
    int x = 0, y = 0, w = 0, h = 0;
    bool hasW = false, hasH = false;
    bool hasX = false, hasY = false;    // 是否显式写了 Left/Top（决定绝对坐标 vs 流式）
    // 行内文本标签（<label>/<span>/<p>…）按内容测出来的宽：不是作者声明。
    // 父是 flex 容器时应当剥掉（CSS 里 flex item 的尺寸由容器定，align-items:stretch 拉满整行）。
    bool autoTextW = false;
    // 按钮/输入框未写 Width，宽度是按内容/控件默认给的自然宽：不是作者声明。
    // 网格单元格应当把它拉满（grid item 默认拉伸），只有显式 Width 才参与 cell 上限截断。
    bool autoW = false;
    int marginL = 0, marginT = 0, marginR = 0, marginB = 0;
    int spacing = 0;                                    // 容器子元素间距（XML Spacing，缺省 0）
    int padL = 0, padT = 0, padR = 0, padB = 0;         // 容器内边距（XML Padding，缺省 0）
    int row = 0, col = 0, rowSpan = 1, colSpan = 1;
    // CSS Grid：作者显式写了 grid-column / grid-row（含 XAML 的 Grid.Column / Grid.Row）。
    // 为 false 时按 HTML 的 auto-placement 规则顺次落格。
    bool hasGridCol = false, hasGridRow = false;
    bool horizontal = false;
    bool hasOrientation = false;                        // 是否显式声明了 Orientation（决定容器排列方向）
    bool autoH = false;                                 // 高度由内容算得（HTML 块容器），重算几何时保留
    // 糖化/默认值表给出的"伪尺寸"标记，跨几何重算保留（autoW/autoH 会在重算时被清掉）：
    // 网格项默认拉伸到单元格，只有作者显式写的宽/高才参与上限截断。
    bool sugarW = false;
    bool sugarH = false;
    // CSS box-sizing:border-box —— Width/Height 指的是"含 padding 的外框"，
    // 解析时已把 padding 从 w/h 里扣掉，二次解析不能重复扣（否则越算越窄）
    bool boxSizingBorder = false;

    // ---- CSS flex（HTML 的“默认排版方式”，在 XML 里用 Display="flex" + 短属性表达）----
    // 容器：Display="flex" / Flex="row|column|row-reverse|column-reverse"
    //       Gap="8" 或 "8 4"、Justify="start|center|end|between|around|evenly"
    //       Align="start|center|end|stretch"、Wrap="0|1"
    // 子项：Grow="1"（按权重吃掉剩余主轴空间）、Self="start|center|end|stretch"
    bool flex = false;
    int flexDir = 0;                                    // 0 row 1 row-reverse 2 column 3 column-reverse
    int flexGapX = 0, flexGapY = 0;                     // 主轴/交叉轴间距
    int flexJustify = 0;                                // 0 start 1 center 2 end 3 between 4 around 5 evenly
    int flexAlign = 3;                                  // 0 start 1 center 2 end 3 stretch
    bool flexWrap = false;
    float flexGrow = 0.0f;                              // 子项 flex-grow
    float flexShrink = 1.0f;                            // 子项 flex-shrink（CSS 默认 1）
    int flexBasis = -1;                                 // 子项 flex-basis，-1 = auto（用自然尺寸）
    // flex-basis 的百分比写法（CSS `flex:0 0 50%` / `flex-basis:50%`），-1 = 未写。
    // 早先这里直接 wcstol，把 "50%" 读成 50px —— 和 width:100% 被误读是同一类真 bug。
    float flexBasisPct = -1.0f;
    int flexSelf = -1;                                  // 子项 align-self，-1 = 跟随容器
    // CSS `margin:0 auto` / `margin-left:auto`：auto 不是数值，必须单独记。
    // 早先 auto 被 wcstol 读成 0，等于整条声明被静默忽略（HTML 里最常见的"水平居中"写不出来）。
    bool marginLAuto = false, marginTAuto = false, marginRAuto = false, marginBAuto = false;
    // style="width:50%" / "height:33%" 的百分比（0~100，-1 = 未写）。100% 仍沿用 fillW/fillH 老语义。
    float pctW = -1.0f, pctH = -1.0f;
    // CSS `position:sticky`：元素随内容滚动，但滚动到距容器顶 stickyTop 处就吸住。
    // XML 侧此前只有 fixed/absolute 的隐式效果，sticky 完全没有。
    bool sticky = false;
    int stickyTop = 0;
    // 这个元素实例化出来的控件（instantiate 时回填）：flex 容器若已实例化成控件层
    // 容器（XmlFlexPanel），排版就交给控件自己的算法，XmlCore 只做参数翻译。
    void* control = nullptr;

    // CSS `position:absolute; inset:0`：撑满父内框（inset:0 翻译层写入，不暴露 XML 属性）
    bool fillParent = false;
    int fillMargin = 0;
    // Layout.fillWidth / Layout.fillHeight：占满容器，或在同类项之间均分剩余空间
    bool fillW = false, fillH = false;
    // Layout.alignment：0=左/上（默认） 1=右/下 2=居中 3=填满 4=基线(Qt.AlignBaseline)
    int alignX = 0, alignY = 0;
    // delegate 行模板（XItemView）：模板节点留一份拷贝，children 里是按行实例化出来的副本
    bool delegateRow = false;
    std::vector<XmlElement> delegateTemplate;

    // 布局结果（运行时计算）
    int previewX = 0, previewY = 0, previewW = 0, previewH = 0;
    // flex 子项的「相对容器」矩形：LayoutFlex 只算这一组，绝对坐标 = 容器原点 + rel。
    // 规则（P0）：flex 里谁排子项只有一个决策者 —— 控件层容器；XmlCore 只产出相对值。
    int relX = 0, relY = 0, relW = 0, relH = 0;

    // 由控件模板实例化出来的装饰子元素（布局按容器对待，但本身不算文档结构）
    bool templateChild = false;
    // 该节点的属性来自 Style/Setter 而非本地声明（用于设计器区分显示，可忽略）
    bool styled = false;

    XML_CORE_API bool has(const wchar_t* key) const;
    XML_CORE_API std::wstring get(const wchar_t* key) const;
    XML_CORE_API void set(const wchar_t* key, const std::wstring& v);

    std::vector<std::pair<std::wstring, std::wstring>> gridRows;   // RowDefinitions
    std::vector<std::pair<std::wstring, std::wstring>> gridCols;   // ColumnDefinitions
};

// ---------------------------------------------------------------------------
// IXmlControl：控件统一接口（所有接口由 C++ 完成）
// ---------------------------------------------------------------------------
class IXmlControl
{
public:
    virtual ~IXmlControl() = default;

    virtual const wchar_t* type() const = 0;

    // 属性读写（反射式）
    virtual bool setProperty(const wchar_t* name, const std::wstring& value) = 0;
    virtual std::wstring getProperty(const wchar_t* name) const = 0;
    virtual std::vector<XmlProperty> properties() const = 0;

    // 事件绑定：kind 为 "click"/"change"/"select"；callback 为宿主回调句柄（由宿主解释）
    virtual bool bindEvent(const wchar_t* kind, void* callback) = 0;

    // 布局
    virtual void setBounds(int x, int y, int w, int h) = 0;
    virtual void getBounds(int& x, int& y, int& w, int& h) const = 0;

    // 创建/销毁：parentHandle 为宿主提供的 HWND（void*）
    virtual bool create(void* parentHandle) = 0;
    virtual void destroy() = 0;
    virtual void* nativeHandle() const = 0;
};

// ---------------------------------------------------------------------------
// XmlControlRegistry：控件注册表（支持自定义控件）
// ---------------------------------------------------------------------------
class XmlControlRegistry
{
public:
    using Factory = std::function<IXmlControl*()>;

    XML_CORE_API static XmlControlRegistry& instance();

    XML_CORE_API bool registerFactory(const wchar_t* tag, Factory factory);
    XML_CORE_API bool isKnown(const wchar_t* tag) const;
    XML_CORE_API IXmlControl* create(const wchar_t* tag) const;
    XML_CORE_API std::vector<std::wstring> tags() const; // 全部已注册标签（供设计器控件箱）

private:
    XmlControlRegistry() = default;
    std::vector<std::pair<std::wstring, Factory>> m_factories;
};

// ---------------------------------------------------------------------------
// 控件属性 schema（S2）：把注册表里每个控件的主标签与其 properties() 导出成 JSON。
// 设计器属性面板、Python 内省、文档生成共用这一份，不再各自维护一份属性清单。
// 结构：{"version":1,"controls":[{"tag":"Button","properties":[
//         {"name":"Text","type":"String","readOnly":false,"enum":""}]}]}
// 同一实现类的多个标签只出一条（取注册顺序里的第一个为主标签）。
// ---------------------------------------------------------------------------
XML_CORE_API std::string XmlPropertySchemaJson();

// ---------------------------------------------------------------------------
// 样式 / 控件模板 / 属性绑定
//
// XML 声明层的三件事，全部由核心层解析，任何渲染端都可直接使用：
//   Style       —— <Style x:Key="...."> + <Setter Property="..." Value="..."/>
//                  （支持 TargetType、BasedOn、隐式样式即无 x:Key 时按类型匹配）
//   Template    —— <ControlTemplate x:Key="..."> 一段 XML 子树，用
//                  {TemplateBinding Prop} 把控件自身属性注入视觉树
//   绑定        —— 属性值写 {Binding Src.Prop}，核心层记录绑定关系并做一次
//                  静态求值；运行时可在值变化后重跑 applyBindings
// ---------------------------------------------------------------------------
struct XmlSetter
{
    std::wstring prop;
    std::wstring value;
};

// 属性触发器：元素当前属性值 == value 时，额外叠加一组 Setter
// （XML 里写 <Trigger Property="Checked" Value="True"> + <Setter .../>）
struct XmlTrigger
{
    std::wstring prop;
    std::wstring value;
    std::vector<XmlSetter> setters;
};

struct XmlStyle
{
    std::wstring key;          // x:Key（空 = 隐式样式，按 TargetType 匹配）
    std::wstring targetType;   // 适用控件标签
    std::wstring basedOn;      // BasedOn 指向的样式键
    std::vector<XmlSetter> setters;
    std::vector<XmlTrigger> triggers;   // BasedOn 链上合并
};

struct XmlBinding
{
    std::wstring targetName;
    std::wstring targetProp;
    std::wstring sourceName;   // 空 = 同一元素
    std::wstring sourceProp;
    bool twoWay = false;       // {Binding ..., Mode=TwoWay}：目标改值回写源
};

// {StaticResource Key} / Key  ->  取出 Key
XML_CORE_API bool xmlIsResourceRef(const std::wstring& value, std::wstring& key);
// {Binding Src.Prop} / {Bind Src.Prop}  ->  取出 Src 与 Prop（Src 可省略）
XML_CORE_API bool xmlIsBindingExpr(const std::wstring& value, std::wstring& source,
                                     std::wstring& prop);
// 内置表达式判定：{a.Width / 2 + 8} 这类（不认 Binding/StaticResource/TemplateBinding）
// 宿主需要用它判断"这个属性的值是不是算出来的"，以便决定要不要回写控件
XML_CORE_API bool xmlIsExpression(const std::wstring& value);

// ---------------------------------------------------------------------------
// XmlRuntime：加载 XML -> 布局 -> 实例化控件树
// ---------------------------------------------------------------------------
class XmlRuntime
{
public:
    XML_CORE_API ~XmlRuntime();

    // 加载（解析 + 建树）；实例化由宿主调用 instantiate 完成
    XML_CORE_API bool loadFile(const wchar_t* path, std::wstring& error);
    XML_CORE_API bool loadString(const wchar_t* xml, std::wstring& error);

    // 样式表 / 模板表 / 绑定表（加载时自动采集；也可由设计器直接注入）
    XML_CORE_API void clearStyles();
    XML_CORE_API void addStyle(const XmlStyle& style);
    XML_CORE_API bool getStyle(const std::wstring& key, XmlStyle& out) const;
    XML_CORE_API std::vector<XmlStyle> styles() const;

    XML_CORE_API void addTemplate(const std::wstring& key, const std::vector<XmlElement>& nodes);
    XML_CORE_API bool getTemplate(const std::wstring& key, std::vector<XmlElement>& out) const;
    // 当前模板表里的全部键（含 ResourceDictionary Source 跨文件合并进来的）
    XML_CORE_API std::vector<std::wstring> templateKeys() const;

    XML_CORE_API const std::vector<XmlBinding>& bindings() const;

    // 把样式合并进各节点的属性（本地属性优先）；模板实例化为装饰子元素；
    // 绑定做一次静态求值。三者可单独重跑，用于运行时改样式/改值后刷新。
    XML_CORE_API void applyStyles();
    XML_CORE_API void applyTemplates();
    XML_CORE_API void applyBindings();
    // 内置表达式：{a.Width / 2 + 8} 这类花括号值（前缀不是 Binding/StaticResource/
    // TemplateBinding 时按表达式求值），结果写进有效属性，读取路径与绑定一致
    XML_CORE_API void applyExpressions();
    // 便捷：三件套一次做完
    XML_CORE_API void applyDeclarations();

    // 写属性（按元素 name 定位，空 = 根），并即时刷新绑定：
    // 目标处于 TwoWay 绑定时回写源，随后重算全表
    XML_CORE_API bool setProperty(const std::wstring& name, const std::wstring& prop,
                                   const std::wstring& value);

    // 按 x:Name 取节点（空 name = 根），找不到返回 nullptr
    XML_CORE_API XmlElement* elementByName(const std::wstring& name);
    // 控件层 flex：宿主把控件挂到元素之后调用，容器即接管自己的子项并重排一次。
    // 只在宿主创建完该元素的整棵子树后调用才有效（子项 id 这时才齐）。
    XML_CORE_API bool attachFlexContainer(XmlElement& el);
    // 节点标签归一后的本意名（小写，如 button / itemview）
    XML_CORE_API std::wstring kindOfElement(const std::wstring& name);
    // QML delegate 重展开：Model 改变后按新行数重建行元素。
    // 只动元素树，行控件的销毁/重建由宿主负责（宿主知道 HWND 与 id 的对应）
    XML_CORE_API bool refreshItemView(const std::wstring& name);

    // 布局：按根矩形计算所有节点 preview 矩形
    XML_CORE_API void layout(int rootX, int rootY, int rootW, int rootH);
    // 实例化：对文档树每个节点调用注册表 create + 属性/布局。
    // parent 为根控件宿主 HWND；返回根控件（调用方持有，.reset() 销毁）。
    // 每创建一个控件即回调 onControl(el, ctrl)，宿主在此绑定事件（如解析回调名）、
    // 记录 name->控件 映射、处理需宿主上下文才能解释的属性。
    using ControlCallback = std::function<void(XmlElement&, IXmlControl*)>;
    XML_CORE_API void setControlCallback(ControlCallback cb);
    XML_CORE_API IXmlControl* instantiate(void* parent);

    // 树访问
    XML_CORE_API XmlElement* root();
    XML_CORE_API const XmlElement* root() const;

    // 保存（XML 序列化）
    XML_CORE_API bool save(const wchar_t* path, std::wstring& error) const;
    // 返回当前文档的 XML 序列化文本（设计器底部代码视图用）
    XML_CORE_API std::wstring toXml() const;

    // 清空文档（重建空树）
    XML_CORE_API void reset();

private:
    std::unique_ptr<XmlElement> m_root;
    // 实例化时创建的控件（与树节点一一对应，用于销毁）
    std::vector<std::unique_ptr<IXmlControl>> m_controls;
    // 节点 -> 控件实例：运行时改属性（如 QML 的 model）要同步到控件本身
    std::map<XmlElement*, IXmlControl*> m_ctrlByElement;
    ControlCallback m_onControl;
    std::vector<XmlStyle> m_styles;
    std::vector<std::pair<std::wstring, std::vector<XmlElement>>> m_templates;
    std::vector<XmlBinding> m_bindings;
};

// ---------------------------------------------------------------------------
// 便捷工具
// ---------------------------------------------------------------------------
XML_CORE_API std::wstring xmlColorToString(COLORREF c);
XML_CORE_API bool xmlParseColor(const std::wstring& s, COLORREF& out);
XML_CORE_API std::vector<std::wstring> xmlSplit(const std::wstring& s, wchar_t sep);

// 对任意子树做布局（宿主处理 PaletteSet 多页等场景时使用）
XML_CORE_API void xmlLayoutSubtree(XmlElement& root, int rootX, int rootY, int rootW, int rootH);

// 内容自然高：容器没写 Height 时"按子项排出来"有多高（横排取最高，竖排累加 + 间距 + 内边距）。
// 片段根自动定窗口高度、横排容器不纵向撑满，都走这一个量法。
XML_CORE_API int xmlNaturalHeight(const XmlElement& el);

// HTML 词汇糖：把 <div>/<p>/<input type="…">/<select><option>/<table><tr><td> 这类
// HTML 标签就地改写成对应的 X 控件（div→XStackPanel、p→XLabel、input→XTextField…
// select 的 option 汇成 Items、table 的行列汇成 Grid 定义 + 单元格）。
// 解析后、布局前调用一次；XAML 写法（X 前缀 / 大写原名）不受影响。
XML_CORE_API void xmlApplyHtmlSugar(XmlElement& root);

// ---------------------------------------------------------------------------
// 布局缺省值（由 control_catalog.xml 注入，避免 C++ 里硬编码控件尺寸等常量）
// ---------------------------------------------------------------------------
// 控件缺省尺寸：catalog 的 objectinfo w/h
XML_CORE_API void xmlSetDefaultSize(const wchar_t* tag, int w, int h);
XML_CORE_API bool xmlGetDefaultSize(const wchar_t* tag, int& w, int& h);
// 星号轨道最小像素（剩余空间不足时每条星号分配的兜底宽度），缺省 30
XML_CORE_API void xmlSetStarMinPx(int px);
// GroupBox 边框物理内区（子控件坐标原点偏移），缺省 12/24
XML_CORE_API void xmlSetGroupInset(int left, int top);
