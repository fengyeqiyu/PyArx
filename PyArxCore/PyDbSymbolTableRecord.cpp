#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSymbolTableRecord.h"

PyWrapper_Cpp(DbSymbolTableRecord);

bool  PyDbSymbolTableRecord::setName(const AcString& name) { if (!impObj()) return false; return PyWarnIfError(impObj()->setName(name.kwszPtr()), L"AcDbSymbolTableRecord::setName"); }
AcString PyDbSymbolTableRecord::getName() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->getName(s), L"AcDbSymbolTableRecord::getName")) return AcString(); return s; }
bool PyDbSymbolTableRecord::isDependent() const { if (!impObj()) return false; return impObj() ? impObj()->isDependent() : false; }
bool PyDbSymbolTableRecord::isResolved() const { if (!impObj()) return false; return impObj() ? impObj()->isResolved() : false; }
bool PyDbSymbolTableRecord::isRenamable() const { if (!impObj()) return false; return impObj() ? impObj()->isRenamable() : false; }
bool PyDbSymbolTableRecord::upgradeOpen() { if (!impObj()) return false; return PyWarnIfError(impObj()->upgradeOpen(), L"SymbolTableRecord.upgradeOpen"); }

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
