#pragma once
// XamlCodeEditor —— XAML <CodeEditor> 的注册级实现。
//
// 与 CodeEditor.h 的分工：
//   * CodeEditor.h   = 独立控件（自带窗口 + D2D 自绘 + 智能提示），设计器/宿主直接 new 来用；
//   * 本文件         = 控件库注册表里的那一个"CodeEditor 标签"，负责 XAML 属性 -> 宿主调用。
//
// 属性（都走基类的 ownProperties 契约，Level / TabIndex / Theme 由基类统一提供）：
//   Content / Text   源码文本
//   Syntax           语法：XAML（默认）/ PYTHON
//   IsReadOnly       只读
//
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"
#include <algorithm>

class XAML_CORE_API XamlCodeEditor : public XamlMfcControlBase
{

public:
    XamlCodeEditor() : XamlMfcControlBase(L"codeeditor") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;


        std::vector<XamlProperty> ownProperties() const override;


        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;


        std::wstring getOwnProperty(const wchar_t* name) const override;


private:
    std::wstring syntaxName() const { return m_python ? L"python" : L"xaml"; }

    std::wstring m_text;
    bool m_readOnly = false;
    bool m_python = false;
};
