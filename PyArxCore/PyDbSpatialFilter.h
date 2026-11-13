#pragma once
#include "stdafx.h"
#include "PyDbFilter.h"

// AcDbSpatialFilter : public AcDbFilter   (dbspfilt.h, line 31)
class PyDbSpatialFilter : public PyDbFilter {
public:
	PyWrapper_H(DbSpatialFilter)
	PyDbSpatialFilter(AcDbSpatialFilter* p) : PyDbFilter(p) {}
};

void bind_PyDbSpatialFilter(pym& m);
