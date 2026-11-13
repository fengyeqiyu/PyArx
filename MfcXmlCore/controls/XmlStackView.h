#pragma once
#include "XmlMfcControlBase.h"
#include "XmlContentParse.h"
#include "XmlPageView.h"

// XmlStackView
class XML_CORE_API XmlStackView : public XmlPageView
{

public:
    XmlStackView() : XmlPageView(L"stackview") {}
};
