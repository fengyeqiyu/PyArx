#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"
#include "XamlPageView.h"

// XamlSwipeView
class XAML_CORE_API XamlSwipeView : public XamlPageView
{

public:
    XamlSwipeView() : XamlPageView(L"swipeview") {}
};
