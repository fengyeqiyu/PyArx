#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"
#include "XamlPageView.h"

// XamlPage
class XAML_CORE_API XamlPage : public XamlPageView
{

public:
    XamlPage() : XamlPageView(L"page") {}
};
