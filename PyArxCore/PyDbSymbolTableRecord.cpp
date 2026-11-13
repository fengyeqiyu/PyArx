#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSymbolTableRecord.h"

PyWrapper_Cpp(DbSymbolTableRecord);

bool  PyDbSymbolTableRecord::setName(const AcString& name) { return PyWarnIfError(impObj()->setName(name.kwszPtr()), L"AcDbSymbolTableRecord::setName"); }
AcString PyDbSymbolTableRecord::getName() const { AcString s; if(!PyWarnIfError(impObj()->getName(s), L"AcDbSymbolTableRecord::getName")) return AcString(); return s; }
bool PyDbSymbolTableRecord::isDependent() const { return impObj()->isDependent(); }
bool PyDbSymbolTableRecord::isResolved() const { return impObj()->isResolved(); }
bool PyDbSymbolTableRecord::isRenamable() const { return impObj()->isRenamable(); }
bool PyDbSymbolTableRecord::upgradeOpen() { return PyWarnIfError(impObj()->upgradeOpen(), L"SymbolTableRecord.upgradeOpen"); }

void bind_PyDbSymbolTableRecord(pym& m) {
	py::class_<PyDbSymbolTableRecord, PyDbObject>(m, "PyDbSymbolTableRecord")
		.def(py::init<AcDbSymbolTableRecord*>(), py::arg("p"))
		PyWrapper_BIND(DbSymbolTableRecord)
		.def("setName", &PyDbSymbolTableRecord::setName, py::arg("name"))
		.def("getName", &PyDbSymbolTableRecord::getName)
		.def("isDependent", &PyDbSymbolTableRecord::isDependent)
		.def("isResolved", &PyDbSymbolTableRecord::isResolved)
		.def("isRenamable", &PyDbSymbolTableRecord::isRenamable)
		.def("upgradeOpen", &PyDbSymbolTableRecord::upgradeOpen, "将打开模式从读升级为写");
}
