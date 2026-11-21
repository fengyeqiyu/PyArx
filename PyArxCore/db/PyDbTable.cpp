#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbTable.h"

PyWrapper_Cpp(DbTable)
AcString PyDbTable::toString() const { return AcString(std::format("AcDbTable[wrapper]").c_str(), AcString::Utf8); }
Adesk::UInt32 PyDbTable::numRows() const { if (!impObj()) return 0; return impObj()->numRows(); }

Adesk::UInt32 PyDbTable::numColumns() const { if (!impObj()) return 0; return impObj()->numColumns(); }

bool  PyDbTable::setSize(int r, int c) { if (!impObj()) return false; return PyWarnIfError(impObj()->setSize(r, c), L"AcDbTable::setSize"); }
double PyDbTable::width() const { if (!impObj()) return 0.0; return impObj()->width(); }
bool  PyDbTable::setWidth(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setWidth(v), L"AcDbTable::setWidth"); }
double PyDbTable::height() const { if (!impObj()) return 0.0; return impObj()->height(); }
bool  PyDbTable::setHeight(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setHeight(v), L"AcDbTable::setHeight"); }
double PyDbTable::columnWidth(int col) const { if (!impObj()) return 0.0; return impObj()->columnWidth(col); }
bool  PyDbTable::setColumnWidth(int col, double w) { if (!impObj()) return false; return PyWarnIfError(impObj()->setColumnWidth(col, w), L"AcDbTable::setColumnWidth"); }
double PyDbTable::rowHeight(int row) const { if (!impObj()) return 0.0; return impObj()->rowHeight(row); }
bool  PyDbTable::setRowHeight(int row, double h) { if (!impObj()) return false; return PyWarnIfError(impObj()->setRowHeight(row, h), L"AcDbTable::setRowHeight"); }
AcString PyDbTable::textString(int row, int col) const { if (!impObj()) return AcString(); const ACHAR* s = impObj()->textString(row, col); return s ? AcString(s) : AcString(); }
bool  PyDbTable::setTextString(int row, int col, const AcString& t) { if (!impObj()) return false; return PyWarnIfError(impObj()->setTextString(row, col, t.kwszPtr()), L"AcDbTable::setTextString"); }
double PyDbTable::textHeight(int row, int col) const { if (!impObj()) return 0.0; return impObj()->textHeight(row, col); }
bool  PyDbTable::setTextHeight(int row, int col, double h) { if (!impObj()) return false; return PyWarnIfError(impObj()->setTextHeight(row, col, h), L"AcDbTable::setTextHeight"); }
bool  PyDbTable::insertColumns(int col, double w, int n) { if (!impObj()) return false; return PyWarnIfError(impObj()->insertColumns(col, w, n), L"AcDbTable::insertColumns"); }
bool  PyDbTable::deleteColumns(int col, int n) { if (!impObj()) return false; return PyWarnIfError(impObj()->deleteColumns(col, n), L"AcDbTable::deleteColumns"); }
bool  PyDbTable::insertRows(int row, double h, int n) { if (!impObj()) return false; return PyWarnIfError(impObj()->insertRows(row, h, n), L"AcDbTable::insertRows"); }
bool  PyDbTable::deleteRows(int row, int n) { if (!impObj()) return false; return PyWarnIfError(impObj()->deleteRows(row, n), L"AcDbTable::deleteRows"); }
bool  PyDbTable::mergeCells(int a, int b, int c, int d) { if (!impObj()) return false; return PyWarnIfError(impObj()->mergeCells(a, b, c, d), L"AcDbTable::mergeCells"); }
bool  PyDbTable::unmergeCells(int a, int b, int c, int d) { if (!impObj()) return false; return PyWarnIfError(impObj()->unmergeCells(a, b, c, d), L"AcDbTable::unmergeCells"); }
bool PyDbTable::isMergedCell(int row, int col) const { if (!impObj()) return false; return impObj()->isMergedCell(row, col); }
bool PyDbTable::isRegenerateTableSuppressed() const { if (!impObj()) return false; return impObj()->isRegenerateTableSuppressed(); }
void PyDbTable::suppressRegenerateTable(bool v) { if (!impObj()) return; impObj()->suppressRegenerateTable(v); }
bool  PyDbTable::setPosition(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setPosition(p), L"AcDbTable::setPosition"); }
int PyDbTable::flowDirection() const { if (!impObj()) return 0; return static_cast<int>(impObj()->flowDirection()); }
bool  PyDbTable::setFlowDirection(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setFlowDirection(static_cast<AcDb::FlowDirection>(v)), L"AcDbTable::setFlowDirection"); }
bool  PyDbTable::generateLayout() { if (!impObj()) return false; return PyWarnIfError(impObj()->generateLayout(), L"AcDbTable::generateLayout"); }
uint64_t PyDbTable::tableStyleId() const { if (!impObj()) return 0ULL; return static_cast<uint64_t>(impObj()->tableStyle().asOldId()); }
bool  PyDbTable::setNumRows(int n) { if (!impObj()) return false; return PyWarnIfError(impObj()->setNumRows(n), L"AcDbTable::setNumRows"); }
bool  PyDbTable::setNumColumns(int n) { if (!impObj()) return false; return PyWarnIfError(impObj()->setNumColumns(n), L"AcDbTable::setNumColumns"); }
bool  PyDbTable::setTableStyle(uint64_t id) { if (!impObj()) return false; AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::IntDbId>(id)); return PyWarnIfError(impObj()->setTableStyle(oid), L"AcDbTable::setTableStyle"); }
bool  PyDbTable::recomputeTableBlock(bool forceUpdate) { if (!impObj()) return false; return PyWarnIfError(impObj()->recomputeTableBlock(forceUpdate), L"AcDbTable::recomputeTableBlock"); }
AcGeVector3d PyDbTable::direction() const { if (!impObj()) return AcGeVector3d(); return impObj()->direction(); }
bool  PyDbTable::setDirection(const AcGeVector3d& v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDirection(v), L"AcDbTable::setDirection"); }
double PyDbTable::rotation() const { if (!impObj()) return 0.0; return impObj()->rotation(); }
bool  PyDbTable::setRotation(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setRotation(v), L"AcDbTable::setRotation"); }

void bind_PyDbTable(pym& m) {
	py::class_<PyDbTable, PyDbBlockReference>(m, "PyDbTable")
		.def(py::init<>())
		.def(py::init<AcDbTable*>(), py::arg("object"))
		.def("numRows", &PyDbTable::numRows)
		.def("numColumns", &PyDbTable::numColumns)
		.def("setSize", &PyDbTable::setSize, py::arg("nRows"), py::arg("nCols"))
		.def("width", &PyDbTable::width)
		.def("setWidth", &PyDbTable::setWidth, py::arg("v"))
		.def("height", &PyDbTable::height)
		.def("setHeight", &PyDbTable::setHeight, py::arg("v"))
		.def("columnWidth", &PyDbTable::columnWidth, py::arg("col"))
		.def("setColumnWidth", &PyDbTable::setColumnWidth, py::arg("col"), py::arg("width"))
		.def("rowHeight", &PyDbTable::rowHeight, py::arg("row"))
		.def("setRowHeight", &PyDbTable::setRowHeight, py::arg("row"), py::arg("height"))
		.def("textString", &PyDbTable::textString, py::arg("row"), py::arg("col"))
		.def("setTextString", &PyDbTable::setTextString, py::arg("row"), py::arg("col"), py::arg("text"))
		.def("textHeight", &PyDbTable::textHeight, py::arg("row"), py::arg("col"))
		.def("setTextHeight", &PyDbTable::setTextHeight, py::arg("row"), py::arg("col"), py::arg("h"))
		.def("insertColumns", &PyDbTable::insertColumns, py::arg("col"), py::arg("width"), py::arg("nCols") = 1)
		.def("deleteColumns", &PyDbTable::deleteColumns, py::arg("col"), py::arg("nCols") = 1)
		.def("insertRows", &PyDbTable::insertRows, py::arg("row"), py::arg("height"), py::arg("nRows") = 1)
		.def("deleteRows", &PyDbTable::deleteRows, py::arg("row"), py::arg("nRows") = 1)
		.def("mergeCells", &PyDbTable::mergeCells, py::arg("minRow"), py::arg("maxRow"), py::arg("minCol"), py::arg("maxCol"))
		.def("unmergeCells", &PyDbTable::unmergeCells, py::arg("minRow"), py::arg("maxRow"), py::arg("minCol"), py::arg("maxCol"))
		.def("isMergedCell", &PyDbTable::isMergedCell, py::arg("row"), py::arg("col"))
		.def("isRegenerateTableSuppressed", &PyDbTable::isRegenerateTableSuppressed)
		.def("suppressRegenerateTable", &PyDbTable::suppressRegenerateTable, py::arg("v"))
		.def("setPosition", &PyDbTable::setPosition, py::arg("p"))
		.def("flowDirection", &PyDbTable::flowDirection)
		.def("setFlowDirection", &PyDbTable::setFlowDirection, py::arg("v"))
		.def("generateLayout", &PyDbTable::generateLayout)
		.def("tableStyleId", &PyDbTable::tableStyleId)
		.def("setNumRows", &PyDbTable::setNumRows, py::arg("nRows"))
		.def("setNumColumns", &PyDbTable::setNumColumns, py::arg("nCols"))
		.def("setTableStyle", &PyDbTable::setTableStyle, py::arg("id"))
		.def("recomputeTableBlock", &PyDbTable::recomputeTableBlock, py::arg("forceUpdate") = true)
		.def("direction", &PyDbTable::direction)
		.def("setDirection", &PyDbTable::setDirection, py::arg("v"))
		.def("rotation", &PyDbTable::rotation)
		.def("setRotation", &PyDbTable::setRotation, py::arg("v"))
		PyWrapper_BIND(DbTable)
		.def("__repr__", &PyDbTable::toString)
		// ---- PyRx getter 对齐补齐（自动生成，纯读无副作用） ----
		.def("breakSpacing", [](const PyDbTable& o) -> double { if (o.isNull()) return 0.0; return o.impObj()->breakSpacing(); })
		.def("hasSubSelection", [](const PyDbTable& o) -> bool { if (o.isNull()) return false; return o.impObj()->hasSubSelection(); })
		.def("horzCellMargin", [](const PyDbTable& o) -> double { if (o.isNull()) return 0.0; return o.impObj()->horzCellMargin(); })
		.def("isBreakEnabled", [](const PyDbTable& o) -> bool { if (o.isNull()) return false; return o.impObj()->isBreakEnabled(); })
		.def("isHeaderSuppressed", [](const PyDbTable& o) -> bool { if (o.isNull()) return false; return o.impObj()->isHeaderSuppressed(); })
		.def("isTitleSuppressed", [](const PyDbTable& o) -> bool { if (o.isNull()) return false; return o.impObj()->isTitleSuppressed(); })
		.def("minimumTableHeight", [](const PyDbTable& o) -> double { if (o.isNull()) return 0.0; return o.impObj()->minimumTableHeight(); })
		.def("minimumTableWidth", [](const PyDbTable& o) -> double { if (o.isNull()) return 0.0; return o.impObj()->minimumTableWidth(); })
		.def("tableStyle", [](const PyDbTable& o) -> uint64_t { auto id = o.impObj()->tableStyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); })
		.def("vertCellMargin", [](const PyDbTable& o) -> double { if (o.isNull()) return 0.0; return o.impObj()->vertCellMargin(); })
		;
}

