#pragma once
// PyMfcXaml 统一运行时核心（C++，无 MFC 依赖）
//
// 原则：
//   * 所有界面代码由 XAML 声明实现；所有接口由 C++ 完成。
//   * 控件通过 XamlControlRegistry 注册（支持自定义控件），不再硬编码 switch。
//   * 属性通过 XamlProperty 元数据反射，设计器属性面板据此动态生成。
//   * IXamlControl 是控件统一接口，宿主（PyMfc / 设计器）提供具体实现与工厂。
//
// 线程安全：运行时核心不持有全局可变状态（注册表为进程级单例，加载/实例化局部）。
// 编码：本文件 UTF-8 with BOM。

#include <windows.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef PYMFCXAML_EXPORTS
#define XAML_CORE_API __declspec(dllexport)
#else
#define XAML_CORE_API __declspec(dllimport)
#endif

// ---------------------------------------------------------------------------
// 属性元数据
// ---------------------------------------------------------------------------
enum class XamlPropType
{
    Int,      // 整数
    String,   // 字符串
    Bool,     // "1"/"0"/"true"/"false"
    Color,    // "#RRGGBB" / "RGB(r,g,b)"
    Enum,     // enumValues 用 "a|b|c"
    Items,    // 条目集（ListBox/ComboBox Items="a|b|c"）
    // 第二批：控件属性表（XamlTypes.h）用到的语义类型。
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

struct XamlProperty
{
    const wchar_t* name = nullptr;
    XamlPropType type = XamlPropType::String;
    bool readable = true;
    bool writable = true;
    const wchar_t* enumValues = nullptr; // Enum 类型时用
    const wchar_t* aliases = nullptr;    // 额外写法，"Text|Caption"，'|' 分隔（大小写不敏感）
};

// ---------------------------------------------------------------------------
// XamlElement：统一节点模型（文档树）
// ---------------------------------------------------------------------------
class XamlElement
{
public:
    std::wstring tag;                                   // 标签名（Button/Grid/...）
    std::wstring name;                                  // x:Name / Name
    std::wstring text;                                  // Content/Text/Header
    std::wstring icon;                                  // Icon（res/xxx.png，相对 exe 目录）
    std::vector<std::pair<std::wstring, std::wstring>> attrs; // 全部 XAML 属性（有序）
    // 由 Style / 绑定注入的"有效属性"：读取时优先本地 attrs，其次这里；
    // 写回文档时只看 attrs，所以样式不会污染 XAML 源码
    std::vector<std::pair<std::wstring, std::wstring>> styledAttrs;
    std::vector<XamlElement> children;                  // 子节点（不含行列定义等元数据）

    // 布局声明值（XAML 属性解析结果）
    int x = 0, y = 0, w = 0, h = 0;
    bool hasW = false, hasH = false;
    bool hasX = false, hasY = false;    // 是否显式写了 Left/Top（决定绝对坐标 vs 流式）
    int marginL = 0, marginT = 0, marginR = 0, marginB = 0;
    int spacing = 0;                                    // 容器子元素间距（XAML Spacing，缺省 0）
    int padL = 0, padT = 0, padR = 0, padB = 0;         // 容器内边距（XAML Padding，缺省 0）
    int row = 0, col = 0, rowSpan = 1, colSpan = 1;
    bool horizontal = false;
    bool hasOrientation = false;                        // 是否显式声明了 Orientation（决定容器排列方向）

    // 布局结果（运行时计算）
    int previewX = 0, previewY = 0, previewW = 0, previewH = 0;

    // 由控件模板实例化出来的装饰子元素（布局按容器对待，但本身不算文档结构）
    bool templateChild = false;
    // 该节点的属性来自 Style/Setter 而非本地声明（用于设计器区分显示，可忽略）
    bool styled = false;

    XAML_CORE_API bool has(const wchar_t* key) const;
    XAML_CORE_API std::wstring get(const wchar_t* key) const;
    XAML_CORE_API void set(const wchar_t* key, const std::wstring& v);

    std::vector<std::pair<std::wstring, std::wstring>> gridRows;   // RowDefinitions
    std::vector<std::pair<std::wstring, std::wstring>> gridCols;   // ColumnDefinitions
};

// ---------------------------------------------------------------------------
// IXamlControl：控件统一接口（所有接口由 C++ 完成）
// ---------------------------------------------------------------------------
class IXamlControl
{
public:
    virtual ~IXamlControl() = default;

    virtual const wchar_t* type() const = 0;

    // 属性读写（反射式）
    virtual bool setProperty(const wchar_t* name, const std::wstring& value) = 0;
    virtual std::wstring getProperty(const wchar_t* name) const = 0;
    virtual std::vector<XamlProperty> properties() const = 0;

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
// XamlControlRegistry：控件注册表（支持自定义控件）
// ---------------------------------------------------------------------------
class XamlControlRegistry
{
public:
    using Factory = std::function<IXamlControl*()>;

    XAML_CORE_API static XamlControlRegistry& instance();

    XAML_CORE_API bool registerFactory(const wchar_t* tag, Factory factory);
    XAML_CORE_API bool isKnown(const wchar_t* tag) const;
    XAML_CORE_API IXamlControl* create(const wchar_t* tag) const;
    XAML_CORE_API std::vector<std::wstring> tags() const; // 全部已注册标签（供设计器控件箱）

private:
    XamlControlRegistry() = default;
    std::vector<std::pair<std::wstring, Factory>> m_factories;
};

// ---------------------------------------------------------------------------
// 样式 / 控件模板 / 属性绑定
//
// XAML 声明层的三件事，全部由核心层解析，任何渲染端都可直接使用：
//   Style       —— <Style x:Key="...."> + <Setter Property="..." Value="..."/>
//                  （支持 TargetType、BasedOn、隐式样式即无 x:Key 时按类型匹配）
//   Template    —— <ControlTemplate x:Key="..."> 一段 XAML 子树，用
//                  {TemplateBinding Prop} 把控件自身属性注入视觉树
//   绑定        —— 属性值写 {Binding Src.Prop}，核心层记录绑定关系并做一次
//                  静态求值；运行时可在值变化后重跑 applyBindings
// ---------------------------------------------------------------------------
struct XamlSetter
{
    std::wstring prop;
    std::wstring value;
};

// 属性触发器：元素当前属性值 == value 时，额外叠加一组 Setter
// （XAML 里写 <Trigger Property="Checked" Value="True"> + <Setter .../>）
struct XamlTrigger
{
    std::wstring prop;
    std::wstring value;
    std::vector<XamlSetter> setters;
};

struct XamlStyle
{
    std::wstring key;          // x:Key（空 = 隐式样式，按 TargetType 匹配）
    std::wstring targetType;   // 适用控件标签
    std::wstring basedOn;      // BasedOn 指向的样式键
    std::vector<XamlSetter> setters;
    std::vector<XamlTrigger> triggers;   // BasedOn 链上合并
};

struct XamlBinding
{
    std::wstring targetName;
    std::wstring targetProp;
    std::wstring sourceName;   // 空 = 同一元素
    std::wstring sourceProp;
    bool twoWay = false;       // {Binding ..., Mode=TwoWay}：目标改值回写源
};

// {StaticResource Key} / Key  ->  取出 Key
XAML_CORE_API bool xamlIsResourceRef(const std::wstring& value, std::wstring& key);
// {Binding Src.Prop} / {Bind Src.Prop}  ->  取出 Src 与 Prop（Src 可省略）
XAML_CORE_API bool xamlIsBindingExpr(const std::wstring& value, std::wstring& source,
                                     std::wstring& prop);
// 内置表达式判定：{a.Width / 2 + 8} 这类（不认 Binding/StaticResource/TemplateBinding）
// 宿主需要用它判断"这个属性的值是不是算出来的"，以便决定要不要回写控件
XAML_CORE_API bool xamlIsExpression(const std::wstring& value);

// ---------------------------------------------------------------------------
// XamlRuntime：加载 XAML -> 布局 -> 实例化控件树
// ---------------------------------------------------------------------------
class XamlRuntime
{
public:
    XAML_CORE_API ~XamlRuntime();

    // 加载（解析 + 建树）；实例化由宿主调用 instantiate 完成
    XAML_CORE_API bool loadFile(const wchar_t* path, std::wstring& error);
    XAML_CORE_API bool loadString(const wchar_t* xml, std::wstring& error);

    // 样式表 / 模板表 / 绑定表（加载时自动采集；也可由设计器直接注入）
    XAML_CORE_API void clearStyles();
    XAML_CORE_API void addStyle(const XamlStyle& style);
    XAML_CORE_API bool getStyle(const std::wstring& key, XamlStyle& out) const;
    XAML_CORE_API std::vector<XamlStyle> styles() const;

    XAML_CORE_API void addTemplate(const std::wstring& key, const std::vector<XamlElement>& nodes);
    XAML_CORE_API bool getTemplate(const std::wstring& key, std::vector<XamlElement>& out) const;
    // 当前模板表里的全部键（含 ResourceDictionary Source 跨文件合并进来的）
    XAML_CORE_API std::vector<std::wstring> templateKeys() const;

    XAML_CORE_API const std::vector<XamlBinding>& bindings() const;

    // 把样式合并进各节点的属性（本地属性优先）；模板实例化为装饰子元素；
    // 绑定做一次静态求值。三者可单独重跑，用于运行时改样式/改值后刷新。
    XAML_CORE_API void applyStyles();
    XAML_CORE_API void applyTemplates();
    XAML_CORE_API void applyBindings();
    // 内置表达式：{a.Width / 2 + 8} 这类花括号值（前缀不是 Binding/StaticResource/
    // TemplateBinding 时按表达式求值），结果写进有效属性，读取路径与绑定一致
    XAML_CORE_API void applyExpressions();
    // 便捷：三件套一次做完
    XAML_CORE_API void applyDeclarations();

    // 写属性（按元素 name 定位，空 = 根），并即时刷新绑定：
    // 目标处于 TwoWay 绑定时回写源，随后重算全表
    XAML_CORE_API bool setProperty(const std::wstring& name, const std::wstring& prop,
                                   const std::wstring& value);

    // 布局：按根矩形计算所有节点 preview 矩形
    XAML_CORE_API void layout(int rootX, int rootY, int rootW, int rootH);
    // 实例化：对文档树每个节点调用注册表 create + 属性/布局。
    // parent 为根控件宿主 HWND；返回根控件（调用方持有，.reset() 销毁）。
    // 每创建一个控件即回调 onControl(el, ctrl)，宿主在此绑定事件（如解析回调名）、
    // 记录 name->控件 映射、处理需宿主上下文才能解释的属性。
    using ControlCallback = std::function<void(XamlElement&, IXamlControl*)>;
    XAML_CORE_API void setControlCallback(ControlCallback cb);
    XAML_CORE_API IXamlControl* instantiate(void* parent);

    // 树访问
    XAML_CORE_API XamlElement* root();
    XAML_CORE_API const XamlElement* root() const;

    // 保存（XAML 序列化）
    XAML_CORE_API bool save(const wchar_t* path, std::wstring& error) const;
    // 返回当前文档的 XAML 序列化文本（设计器底部代码视图用）
    XAML_CORE_API std::wstring toXml() const;

    // 清空文档（重建空树）
    XAML_CORE_API void reset();

private:
    std::unique_ptr<XamlElement> m_root;
    // 实例化时创建的控件（与树节点一一对应，用于销毁）
    std::vector<std::unique_ptr<IXamlControl>> m_controls;
    ControlCallback m_onControl;
    std::vector<XamlStyle> m_styles;
    std::vector<std::pair<std::wstring, std::vector<XamlElement>>> m_templates;
    std::vector<XamlBinding> m_bindings;
};

// ---------------------------------------------------------------------------
// 便捷工具
// ---------------------------------------------------------------------------
XAML_CORE_API std::wstring xamlColorToString(COLORREF c);
XAML_CORE_API bool xamlParseColor(const std::wstring& s, COLORREF& out);
XAML_CORE_API std::vector<std::wstring> xamlSplit(const std::wstring& s, wchar_t sep);

// 对任意子树做布局（宿主处理 PaletteSet 多页等场景时使用）
XAML_CORE_API void xamlLayoutSubtree(XamlElement& root, int rootX, int rootY, int rootW, int rootH);

// ---------------------------------------------------------------------------
// 布局缺省值（由 control_catalog.xml 注入，避免 C++ 里硬编码控件尺寸等常量）
// ---------------------------------------------------------------------------
// 控件缺省尺寸：catalog 的 objectinfo w/h
XAML_CORE_API void xamlSetDefaultSize(const wchar_t* tag, int w, int h);
XAML_CORE_API bool xamlGetDefaultSize(const wchar_t* tag, int& w, int& h);
// 星号轨道最小像素（剩余空间不足时每条星号分配的兜底宽度），缺省 30
XAML_CORE_API void xamlSetStarMinPx(int px);
// GroupBox 边框物理内区（子控件坐标原点偏移），缺省 12/24
XAML_CORE_API void xamlSetGroupInset(int left, int top);
