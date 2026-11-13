#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"
#include "XamlPageView.h"

// XamlStackView
class XAML_CORE_API XamlStackView : public XamlPageView
{

public:
    XamlStackView() : XamlPageView(L"stackview") {}
};
