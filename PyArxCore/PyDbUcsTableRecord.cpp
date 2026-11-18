#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbUcsTableRecord.h"

PyWrapper_Cpp(DbUCSTableRecord);

AcGePoint3d PyDbUCSTableRecord::origin() const { if (!impObj()) return AcGePoint3d(); return impObj() ? impObj()->origin() : AcGePoint3d(); }
bool PyDbUCSTableRecord::setOrigin(const AcGePoint3d& pt) { if (!impObj()) return false; impObj()->setOrigin(pt); return true; }
AcGeVector3d PyDbUCSTableRecord::xAxis() const { if (!impObj()) return AcGeVector3d(); return impObj() ? impObj()->xAxis() : AcGeVector3d(); }
bool PyDbUCSTableRecord::setXAxis(const AcGeVector3d& v) { if (!impObj()) return false; impObj()->setXAxis(v); return true; }
AcGeVector3d PyDbUCSTableRecord::yAxis() const { if (!impObj()) return AcGeVector3d(); return impObj() ? impObj()->yAxis() : AcGeVector3d(); }
bool PyDbUCSTableRecord::setYAxis(const AcGeVector3d& v) { if (!impObj()) return false; impObj()->setYAxis(v); return true; }

AcGePoint3d PyDbUCSTableRecord::ucsBaseOrigin(int view) const {
	if (!impObj()) return AcGePoint3d();
	return impObj()->ucsBaseOrigin(static_cast<AcDb::OrthographicView>(view));
}
bool PyDbUCSTableRecord::setUcsBaseOrigin(const AcGePoint3d& pt, int view) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setUcsBaseOrigin(pt, static_cast<AcDb::OrthographicView>(view)), L"AcDbUCSTableRecord::setUcsBaseOrigin");
}

void bind_PyDbUcsTableRecord(pym& m) {
	py::class_<PyDbUCSTableRecord, PyDbSymbolTableRecord>(m, "PyDbUcsTableRecord")
		.def(py::init<AcDbUCSTableRecord*>(), py::arg("p"))
		PyWrapper_BIND(DbUCSTableRecord)
		.def("origin", &PyDbUCSTableRecord::origin)
		.def("setOrigin", &PyDbUCSTableRecord::setOrigin, py::arg("pt"))
		.def("xAxis", &PyDbUCSTableRecord::xAxis)
		.def("setXAxis", &PyDbUCSTableRecord::setXAxis, py::arg("v"))
		.def("yAxis", &PyDbUCSTableRecord::yAxis)
		.def("setYAxis", &PyDbUCSTableRecord::setYAxis, py::arg("v"))
		.def("ucsBaseOrigin", &PyDbUCSTableRecord::ucsBaseOrigin, py::arg("view"))
		.def("setUcsBaseOrigin", &PyDbUCSTableRecord::setUcsBaseOrigin, py::arg("pt"), py::arg("view"));
}
