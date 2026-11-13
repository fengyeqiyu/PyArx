#pragma once
#include "XmlMfcControlBase.h"
#include "XmlContentParse.h"
#include "XmlPageView.h"

// XmlPage
class XML_CORE_API XmlPage : public XmlPageView
{

public:
    XmlPage() : XmlPageView(L"page") {}
};
