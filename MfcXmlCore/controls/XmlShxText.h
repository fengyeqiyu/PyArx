#pragma once
// XmlShxText —— SHX 文字控件：把 .shx / 大字体排版成折线，交给宿主自绘画布。
//
// XAML 用法：
//   <XShxText x="10" y="10" w="420" h="90"
//             Text="图纸设计 ABC 123"
//             FontFile="romans.shx" BigFontFile="hztxt.shx"
//             FontHeight="36" WidthFactor="1" Oblique="0" Stroke="#1A1A1A"/>
//
// 渲染通路：createNative 建一个自绘画布（addDwgCanvas），再把排版结果转成
// <XScene> 显示列表（XPolyline）用 setDwgXml 推给宿主 —— 不依赖 AutoCAD 的
// 文字引擎，也不需要 TrueType 字体，缩放重绘仍走矢量。
//
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

#include <string>
#include <vector>

class XML_CORE_API XmlShxText : public XmlMfcControlBase
{
public:
    XmlShxText() : XmlMfcControlBase(L"shxtext") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int minValue, int maxValue,
                      int x, int y, int w, int h) override;
    void setBounds(int x, int y, int w, int h) override;

    std::vector<XmlProperty> ownProperties() const override;
    bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;
    std::wstring getOwnProperty(const wchar_t* name) const override;
    void onAppearanceChanged() override;

private:
    void rebuild();
    std::wstring buildScene() const;
    std::wstring resolveFont(const std::wstring& name) const;

    std::wstring m_text = L"SHX 字体渲染 0123456789";
    std::wstring m_fontFile = L"romans.shx";
    std::wstring m_bigFontFile;
    std::wstring m_stroke = L"#1A1A1A";
    double m_fontHeight = 0.0;      // 0 = 按控件高度自动
    double m_widthFactor = 1.0;
    double m_oblique = 0.0;

    int m_w = 0;
    int m_h = 0;
};
