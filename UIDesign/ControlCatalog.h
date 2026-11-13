#pragma once
// ControlCatalog：XML 声明式控件目录（参照 wxFormBuilder objectinfo 机制）
// 用 XML 声明每个控件的 tag / 控件箱显示名 / 分类 / 默认尺寸 / 属性表，
// 驱动设计器的控件箱（Palette）与属性面板（Inspector），支持自定义控件扩展：
// 只需在 control_catalog.xml 增删一个 <objectinfo>，无需改 C++ 硬编码。
#include <vector>
#include "stdafx.h"

namespace uidesign
{
    struct CtrlProp
    {
        CString name;
        CString type;      // text / bool / int / color / enum ...
        CString options;   // enum 候选取值，用 '|' 分隔（如 "Horizontal|Vertical"），空表示自由输入
    };

    struct CtrlTemplate
    {
        CString tag;       // XML tag（Button/TextBox/...）
        CString display;   // 控件箱显示名（与 tag 一致：直接用 XML 标签名，不再用 wx 类名）
        CString category;  // 分类（Buttons/Input Widgets/...）
        int dw = 100;      // 拖入画布默认宽
        int dh = 28;       // 默认高
        std::vector<CtrlProp> props;
    };

    class ControlCatalog
    {
    public:
        bool load(const CString& path);

        std::vector<CString> categories() const;
        std::vector<const CtrlTemplate*> templates(const CString& category) const;

        // display -> 模板（控件箱项点击/拖拽用）
        const CtrlTemplate* byDisplay(const CString& display) const;
        // tag -> 模板（属性面板按节点 tag 找属性表）
        const CtrlTemplate* byTag(const CString& tag) const;

    private:
        std::vector<CtrlTemplate> m_templates;
    };
}
