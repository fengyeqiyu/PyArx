#include "stdafx.h"
#include "PyAdsCode.h"
#include <acadstrc.h>
#include "dbmain.h"
#include "acdb.h"
#include <acdbabb.h>
#include "dbjig.h"
#include "PyDbDatabase.h" // for FindTextOptions
#include "PyAdsTableUpdateEnums.h" // bind_PyUpdateDirection

void bind_PyEnum(pym& m) {
	bind_PyErrorStatus(m);
	bind_PyPromptStatus(m);
	bind_PyOpenMode(m);
	bind_PyCoordAxis(m);
	bind_PyCoordSystem(m);
	bind_PyIntersect(m);
	bind_PyVisibility(m);
	bind_PyDxfCode(m);
	bind_PyLineWeight(m);
	bind_PyPlotStyleNameType(m);
	bind_PyEndCaps(m);
	bind_PyUnitsValue(m);
	bind_PyJoinStyle(m);
	bind_PyMeasurementValue(m);
	bind_PyUpdateOption(m);
	bind_PyUpdateDirection(m);
	bind_FindTextOptions(m);
	bind_JigEnums(m);
	bind_PyExtraEnums(m);
}
