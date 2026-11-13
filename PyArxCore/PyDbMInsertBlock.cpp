#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbMInsertBlock.h"

PyWrapper_Cpp(DbMInsertBlock)
AcString PyDbMInsertBlock::toString() const { return AcString(std::format("AcDbMInsertBlock[wrapper]").c_str(), AcString::Utf8); }
Adesk::UInt16 PyDbMInsertBlock::columns() const { return impObj()->columns(); }

bool  PyDbMInsertBlock::setColumns(Adesk::UInt16 columns) { return PyWarnIfError(impObj()->setColumns(columns), L"AcDbMInsertBlock::setColumns"); }

Adesk::UInt16 PyDbMInsertBlock::rows() const { return impObj()->rows(); }

bool  PyDbMInsertBlock::setRows(Adesk::UInt16 rows) { return PyWarnIfError(impObj()->setRows(rows), L"AcDbMInsertBlock::setRows"); }

double PyDbMInsertBlock::columnSpacing() const { return impObj()->columnSpacing(); }

bool  PyDbMInsertBlock::setColumnSpacing(double spacing) { return PyWarnIfError(impObj()->setColumnSpacing(spacing), L"AcDbMInsertBlock::setColumnSpacing"); }

double PyDbMInsertBlock::rowSpacing() const { return impObj()->rowSpacing(); }

bool  PyDbMInsertBlock::setRowSpacing(double spacing) { return PyWarnIfError(impObj()->setRowSpacing(spacing), L"AcDbMInsertBlock::setRowSpacing"); }

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
