#pragma once
// XmlMfcControlBase —— 所有控件（标准 + 扩展 + 自定义）的唯一基类。
//
// 基类统一承担三件事，子类只写"自己特有"的部分：
//   1. 外观契约（XmlAppearance）：形状 / 颜色 / 风格 / 前置背景 / 后置背景 / 字体 / 对齐；
//   2. 回调契约：XML 上声明的 Click/TextChanged/SelectionChanged... 事件名 + 回调句柄表；
//   3. 通用 IXmlControl 实现：生命周期、bounds、属性面板元数据聚合。
//
// 子类只实现：
//   createNative(...)      创建原生控件
//   ownProperties()        自己特有的属性元数据
//   setOwnProperty/getOwnProperty  自己特有的属性读写
//
// 编码：本文件 UTF-8 with BOM。

#include "XmlCore.h"
#include "XmlMfcHost.h"
#include "XmlPropUtil.h"
#include "XmlEvents.h"       // 事件契约唯一来源（前端声明 ↔ 后端派发）
#include "XmlTypes.h"        // 属性表（S1）：一个属性只定义一次
#include "XmlAppearance.h"   // 外观基元在运行时核心（PyMfcXml），三端共用
#include "XmlPropAliases.h"   // 小写属性名 -> 内部属性名

#include <string>
#include <utility>
#include <vector>

class XML_CORE_API XmlMfcControlBase : public IXmlControl
{
public:
    explicit XmlMfcControlBase(std::wstring type);
    ~XmlMfcControlBase() override = default;

    // ---- IXmlControl ----
    const wchar_t* type() const override;

    // 宿主在 SetControlCallback 中调用：把控件挂到 host，真正创建 Win32 子控件。
    // id 由 guiload 自动分配（1000..65000）；布局矩形来自运行时 preview。
    void attach(IXmlMfcHost* host, int id, int minValue, int maxValue,
                int x, int y, int w, int h);

    bool create(void* parentHandle) override;
    void destroy() override;
    void* nativeHandle() const override;
    void setBounds(int x, int y, int w, int h) override;
    void getBounds(int& x, int& y, int& w, int& h) const override;

    // 属性 = 统一外观属性 + 事件声明 + 子类自有属性
    std::vector<XmlProperty> properties() const override;
    bool setProperty(const wchar_t* name, const std::wstring& value) override;
    std::wstring getProperty(const wchar_t* name) const override;

    // 回调表：XML 声明的事件名（kind -> 函数名）与宿主绑定的回调句柄（kind -> void*）
    bool bindEvent(const wchar_t* kind, void* callback) override;
    void* eventHandler(const wchar_t* kind) const;
    std::wstring eventName(const wchar_t* kind) const;
    void clearHandlers();

    // 本控件"该有的事件"：缺省按 XmlEvents 的类型契约给出；
    // 已迁移到属性表/显式声明的控件可覆盖（S2：事件归控件自己）。
    virtual std::vector<XmlEventKind> events() const { return xmlDefaultEvents(m_type); }

    // 统一外观
    const xmlapp::XmlAppearance& appearance() const { return m_ap; }
    void setAppearance(const xmlapp::XmlAppearance& ap);

    int Id() const { return m_id; }

    // 本控件是不是 flex 容器（XmlFlexPanel）。
    // 加载层据此决定"子控件要不要 SetParent 到容器上"（真实父子层级），
    // 不去 dynamic_cast 具体类型，免得 PyMfcXml 依赖 MfcXmlCore 的控件类头文件。
    virtual bool isFlexContainer() const { return false; }

    // ---- 所有控件共有的基础属性 ----
    // Level：绘图层次，越大越靠上（改它只动 z 序，不动键盘顺序）
    // TabIndex：Tab 键序号，只影响键盘遍历（与 z 序解耦，避免"调叠放就改 Tab"）
    // Theme：主题名，先做声明/回读/下发通路，颜色分配规则后续再定
    int level() const { return m_level; }
    int tabIndex() const { return m_tabIndex; }
    const std::wstring& theme() const { return m_theme; }
    void setLevel(int v);
    void setTabIndex(int v);
    void setTheme(const std::wstring& v);

protected:
    // 子类实现：真正创建 Win32 控件（调用 host 的 Add*）
    virtual void createNative(IXmlMfcHost* host, int id, int minValue, int maxValue,
                              int x, int y, int w, int h) = 0;

    // 子类实现：自己特有的属性（外观/事件已由基类处理）
    // 【旧路径】未迁到属性表的控件继续用这三个钩子；
    // 【新路径】迁到属性表的控件实现 ownPropTable()，三个钩子留空即可。
    virtual std::vector<XmlProperty> ownProperties() const { return {}; }
    virtual bool setOwnProperty(const wchar_t* /*name*/, const std::wstring& /*value*/) { return false; }
    virtual std::wstring getOwnProperty(const wchar_t* /*name*/) const { return {}; }

    // 属性表（S1）：返回类内静态表首地址，count 为条目数；未迁移返回 nullptr。
    // 表里 name/别名/类型/枚举值/读写只写一次，基类统一分派。
    virtual const XmlPropEntry* ownPropTable(size_t& count) const
    {
        count = 0;
        return nullptr;
    }

    // 按表读写；命中返回 true（未命中不动 *ok）
    bool setFromPropTable(const wchar_t* name, const std::wstring& value, bool& handled);
    std::wstring getFromPropTable(const wchar_t* name, bool& handled) const;

    // 布局/结构类属性由运行时消费，控件不认识属正常，不进未知属性诊断
    static bool IsLayoutAttribute(const std::wstring& key);

    // 外观变化后通知宿主重绘
    void pushAppearance();

    // 外观变化后给子类的钩子：需要在"外观字段"落地到自己内部格式的控件
    // （如 RichText 的 FontSize/Foreground 要转成 CHARFORMAT）在这里同步，
    // 默认什么都不做。调用点在 pushAppearance 里，setProperty 写外观后必然经过。
    virtual void onAppearanceChanged() {}

    // 容器类的背景背板钩子：容器本身是原生 HWND、宿主不做 owner-draw，
    // 所以 <div style="background:..."> 的底色必须交给宿主的绘制节点管线来画。
    // 默认空实现（普通控件没有背板），容器控件 override 后按自身 rect 登记。
    virtual void pushBackdrop() {}

    std::wstring m_type;       // 短名（button/label/...）
    IXmlMfcHost* m_host = nullptr;
    int m_id = 0;
    int m_min = 0;
    int m_max = 100;
    bool m_created = false;
    int m_level = 0;           // 绘图层次（越大越上）
    int m_tabIndex = -1;       // Tab 序号（-1 = 未声明，按创建序）
    std::wstring m_theme;      // 主题名（空 = 用宿主默认）
    bool m_enabled = true;     // Enabled="False"（HTML disabled）
    bool m_visible = true;     // Visible="False"（HTML hidden）
    void pushOrder();          // 层次/序号变化后推给宿主
    void pushTheme();          // 主题变化后推给宿主
    void applyEnableState();   // 可用/可见状态变化后推给宿主

    xmlapp::XmlAppearance m_ap;                                 // 外观声明
    std::vector<std::pair<std::wstring, std::wstring>> m_eventNames; // kind -> XML 函数名
    std::vector<std::pair<std::wstring, void*>> m_handlers;          // kind -> 回调句柄
};
