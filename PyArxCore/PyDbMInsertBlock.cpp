#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbMInsertBlock.h"

PyWrapper_Cpp(DbMInsertBlock)
AcString PyDbMInsertBlock::toString() const { return AcString(std::format("AcDbMInsertBlock[wrapper]").c_str(), AcString::Utf8); }
Adesk::UInt16 PyDbMInsertBlock::columns() const { return impObj() ? impObj()->columns() : 0; }

bool  PyDbMInsertBlock::setColumns(Adesk::UInt16 columns) { if (!impObj()) return false; return PyWarnIfError(impObj()->setColumns(columns), L"AcDbMInsertBlock::setColumns"); }

Adesk::UInt16 PyDbMInsertBlock::rows() const { return impObj() ? impObj()->rows() : 0; }

bool  PyDbMInsertBlock::setRows(Adesk::UInt16 rows) { if (!impObj()) return false; return PyWarnIfError(impObj()->setRows(rows), L"AcDbMInsertBlock::setRows"); }

double PyDbMInsertBlock::columnSpacing() const { return impObj() ? impObj()->columnSpacing() : 0.0; }

bool  PyDbMInsertBlock::setColumnSpacing(double spacing) { if (!impObj()) return false; return PyWarnIfError(impObj()->setColumnSpacing(spacing), L"AcDbMInsertBlock::setColumnSpacing"); }

double PyDbMInsertBlock::rowSpacing() const { return impObj() ? impObj()->rowSpacing() : 0.0; }

bool  PyDbMInsertBlock::setRowSpacing(double spacing) { if (!impObj()) return false; return PyWarnIfError(impObj()->setRowSpacing(spacing), L"AcDbMInsertBlock::setRowSpacing"); }

void bind_PyDbMInsertBlock(pym& m) {
	py::class_<PyDbMInsertBlock, PyDbBlockReference>(m, "PyDbMInsertBlock")
		.def(py::init<>())
		.def(py::init<AcDbMInsertBlock*>(), py::arg("object"))
		.def("columns", &PyDbMInsertBlock::columns)
		.def("setColumns", &PyDbMInsertBlock::setColumns, py::arg("columns"))
		.def("rows", &PyDbMInsertBlock::rows)
		.def("setRows", &PyDbMInsertBlock::setRows, py::arg("rows"))
		.def("columnSpacing", &PyDbMInsertBlock::columnSpacing)
		.def("setColumnSpacing", &PyDbMInsertBlock::setColumnSpacing, py::arg("spacing"))
		.def("rowSpacing", &PyDbMInsertBlock::rowSpacing)
		.def("setRowSpacing", &PyDbMInsertBlock::setRowSpacing, py::arg("spacing"))
		PyWrapper_BIND(DbMInsertBlock)
		.def("__repr__", &PyDbMInsertBlock::toString)
		;
}
