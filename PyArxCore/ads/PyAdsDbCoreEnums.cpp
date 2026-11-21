#include "stdafx.h"
#include "PyAdsDbCoreEnums.h"
#include "acdb.h"
void bind_PyOpenMode(pym& m)
{
	py::enum_<AcDb::OpenMode>(m, "OpenMode")
		.value("kForRead", AcDb::kForRead)
		.value("ForRead", AcDb::kForRead) // PyRx 同义名
		.value("kForWrite", AcDb::kForWrite)
		.value("ForWrite", AcDb::kForWrite) // PyRx 同义名
		.value("kForNotify", AcDb::kForNotify)
		.value("ForNotify", AcDb::kForNotify) // PyRx 同义名
		.export_values();
}


void bind_PyCoordAxis(pym& m) {
	py::enum_<AcDb::CoordAxis>(m, "CoordAxis")
		.value("kX", AcDb::kX)
		.value("kY", AcDb::kY)
		.value("kZ", AcDb::kZ)
		.export_values();
}


void bind_PyCoordSystem(pym& m) {
	py::enum_<AcDb::CoordSystem>(m, "CoordSystem")
		.value("kWorldCS", AcDb::kWorldCS)
		.value("kUserCS", AcDb::kUserCS)
		.value("kCurDisplayCS", AcDb::kCurDisplayCS)
		.value("kPaperDisplayCS", AcDb::kPaperDisplayCS)
		.value("kEntityCS", AcDb::kEntityCS)
		.export_values();
}


void bind_PyIntersect(pym& m)
{
	py::enum_<AcDb::Intersect>(m, "Intersect")
		.value("kOnBothOperands", AcDb::kOnBothOperands)
		.value("kExtendThis", AcDb::kExtendThis)
		.value("kExtendArg", AcDb::kExtendArg)
		.value("kExtendBoth", AcDb::kExtendBoth)
		.export_values();

}


void bind_PyVisibility(pym& m) {
	py::enum_<AcDb::Visibility>(m, "Visibility")
		.value("kInvisible", AcDb::kInvisible)
		.value("kVisible", AcDb::kVisible)
		.export_values();
}
