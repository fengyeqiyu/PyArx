#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <Splitter> 分隔条
class XAML_CORE_API XamlSplitter : public XamlMfcControlBase
{

public:
    XamlSplitter() : XamlMfcControlBase(L"splitter") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;
};
