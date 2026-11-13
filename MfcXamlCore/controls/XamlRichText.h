#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <RichText> 富文本编辑
class XAML_CORE_API XamlRichText : public XamlMfcControlBase
{

public:
    XamlRichText() : XamlMfcControlBase(L"richtext") {}

        bool setProperty(const wchar_t* name, const std::wstring& value) override;


protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;


private:
    int m_fontPt = 9;
    bool m_bold = false;
    bool m_italic = false;
    unsigned long m_color = 0xFFFFFF;
};
