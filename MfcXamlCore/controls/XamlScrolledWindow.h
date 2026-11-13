#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <ScrolledWindow> 滚动面板
class XAML_CORE_API XamlScrolledWindow : public XamlMfcControlBase
{

public:
    XamlScrolledWindow() : XamlMfcControlBase(L"scrolledwindow") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;


private:
    void applyRange()
    {
        if (m_host == nullptr || m_maxValue <= m_minValue)
            return;
        m_host->setScrollRange(m_id, m_minValue, m_maxValue);
    }

    int m_minValue = 0;
    int m_maxValue = 0;
};
