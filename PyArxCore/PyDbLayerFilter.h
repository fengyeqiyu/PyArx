#pragma once
#include "stdafx.h"
#include "PyDbFilter.h"

// AcDbLayerFilter : public AcDbFilter   (dblyfilt.h, line 22)
class PyDbLayerFilter : public PyDbFilter {
public:
	PyWrapper_H(DbLayerFilter)
	PyDbLayerFilter(AcDbLayerFilter* p) : PyDbFilter(p) {}
};

void bind_PyDbLayerFilter(pym& m);
