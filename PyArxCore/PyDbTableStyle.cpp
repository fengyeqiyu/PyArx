#include "stdafx.h"
#include "PyErrorStatusException.h"
#include <dbTableStyle.h>
#include "PyDbTableStyle.h"

PyWrapper_Cpp(DbTableStyle);

AcString PyDbTableStyle::getName() const {
	AcString s; if(!PyWarnIfError(impObj()->getName(s), L"AcDbTableStyle::getName")) return AcString(); return s;
}
bool PyDbTableStyle::setName(const AcString& name) { return PyWarnIfError(impObj()->setName(name.kwszPtr()), L"AcDbTableStyle::setName"); }
// isRenamable() const is not exported by the import library; left as skeleton.
AcString PyDbTableStyle::description() const { const ACHAR* p = impObj()->description(); return p ? AcString(p) : AcString(); }
bool PyDbTableStyle::setDescription(const AcString& desc) { return PyWarnIfError(impObj()->setDescription(desc.kwszPtr()), L"AcDbTableStyle::setDescription"); }
int PyDbTableStyle::bitFlags() const { return static_cast<int>(impObj()->bitFlags()); }
bool PyDbTableStyle::setBitFlags(int flags) { return PyWarnIfError(impObj()->setBitFlags(static_cast<Adesk::UInt32>(flags)), L"AcDbTableStyle::setBitFlags"); }
int PyDbTableStyle::flowDirection() const { return static_cast<int>(impObj()->flowDirection()); }
bool PyDbTableStyle::setFlowDirection(int flow) { return PyWarnIfError(impObj()->setFlowDirection(static_cast<AcDb::FlowDirection>(flow)), L"AcDbTableStyle::setFlowDirection"); }
double PyDbTableStyle::textHeight() const { return impObj()->textHeight(); }
bool PyDbTableStyle::setTextHeight(double h) { return PyWarnIfError(impObj()->setTextHeight(h), L"AcDbTableStyle::setTextHeight"); }

uint64_t PyDbTableStyle::textStyle(int rowType) const {
	AcDbObjectId id = impObj()->textStyle(static_cast<AcDb::RowType>(rowType));
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}

bool PyDbTableStyle::setTextStyle(uint64_t id, int rowTypes) {
	AcDbObjectId oid;
	oid.setFromOldId(static_cast<Adesk::IntDbId>(id));
	return PyWarnIfError(impObj()->setTextStyle(oid, rowTypes), L"AcDbTableStyle::setTextStyle");
}

int PyDbTableStyle::alignment(int rowType) const {
	return static_cast<int>(impObj()->alignment(static_cast<AcDb::RowType>(rowType)));
}

bool PyDbTableStyle::setAlignment(int align, int rowTypes) {
	return PyWarnIfError(impObj()->setAlignment(static_cast<AcDb::CellAlignment>(align), rowTypes), L"AcDbTableStyle::setAlignment");
}
int PyDbTableStyle::numCellStyles() const { return impObj()->numCellStyles(); }
bool PyDbTableStyle::isCellStyleInUse(const AcString& cellStyle) const { return impObj()->isCellStyleInUse(cellStyle.kwszPtr()); }

// ---- 颜色 RGB（AcCmColor 未单独封装，按 (r,g,b) 出入参包装）----
std::tuple<int,int,int> PyDbTableStyle::color(int rowType) const {
	AcCmColor c = impObj()->color(static_cast<AcDb::RowType>(rowType));
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
bool PyDbTableStyle::setColor(int r, int g, int b, int rowTypes) {
	AcCmColor c; c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
	return PyWarnIfError(impObj()->setColor(c, rowTypes), L"AcDbTableStyle::setColor");
}
std::tuple<int,int,int> PyDbTableStyle::backgroundColor(int rowType) const {
	AcCmColor c = impObj()->backgroundColor(static_cast<AcDb::RowType>(rowType));
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
bool PyDbTableStyle::setBackgroundColor(int r, int g, int b, int rowTypes) {
	AcCmColor c; c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
	return PyWarnIfError(impObj()->setBackgroundColor(c, rowTypes), L"AcDbTableStyle::setBackgroundColor");
}
bool PyDbTableStyle::isBackgroundColorNone(int rowType) const { return impObj()->isBackgroundColorNone(static_cast<AcDb::RowType>(rowType)); }
bool PyDbTableStyle::setBackgroundColorNone(bool b, int rowTypes) { return PyWarnIfError(impObj()->setBackgroundColorNone(b, rowTypes), L"AcDbTableStyle::setBackgroundColorNone"); }

// ---- 数据类型 / 格式 ----
std::tuple<int,int> PyDbTableStyle::getDataType() const {
	AcValue::DataType dt; AcValue::UnitType ut;
	if(!PyWarnIfError(impObj()->getDataType(dt, ut), L"AcDbTableStyle::getDataType")) return std::make_tuple(0, 0);
	return std::make_tuple((int)dt, (int)ut);
}
bool PyDbTableStyle::setDataType(int dt, int ut) {
	return PyWarnIfError(impObj()->setDataType(static_cast<AcValue::DataType>(dt), static_cast<AcValue::UnitType>(ut)), L"AcDbTableStyle::setDataType");
}
AcString PyDbTableStyle::format() const { const ACHAR* p = impObj()->format(); return p ? AcString(p) : AcString(); }
bool PyDbTableStyle::setFormat(const AcString& fmt) { return PyWarnIfError(impObj()->setFormat(fmt.kwszPtr()), L"AcDbTableStyle::setFormat"); }

// ---- 网格线属性 ----
int PyDbTableStyle::gridLineWeight(int glt, int rowType) const { return (int)impObj()->gridLineWeight(static_cast<AcDb::GridLineType>(glt), static_cast<AcDb::RowType>(rowType)); }
bool PyDbTableStyle::setGridLineWeight(int lw, int glts, int rowTypes) { return PyWarnIfError(impObj()->setGridLineWeight(static_cast<AcDb::LineWeight>(lw), glts, rowTypes), L"AcDbTableStyle::setGridLineWeight"); }
std::tuple<int,int,int> PyDbTableStyle::gridColor(int glt, int rowType) const {
	AcCmColor c = impObj()->gridColor(static_cast<AcDb::GridLineType>(glt), static_cast<AcDb::RowType>(rowType));
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
bool PyDbTableStyle::setGridColor(int r, int g, int b, int glts, int rowTypes) {
	AcCmColor c; c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
	return PyWarnIfError(impObj()->setGridColor(c, glts, rowTypes), L"AcDbTableStyle::setGridColor");
}
int PyDbTableStyle::gridVisibility(int glt, int rowType) const { return (int)impObj()->gridVisibility(static_cast<AcDb::GridLineType>(glt), static_cast<AcDb::RowType>(rowType)); }
bool PyDbTableStyle::setGridVisibility(int vis, int glts, int rowTypes) { return PyWarnIfError(impObj()->setGridVisibility(static_cast<AcDb::Visibility>(vis), glts, rowTypes), L"AcDbTableStyle::setGridVisibility"); }

// ---- 边距 / 标题与表头抑制 ----
double PyDbTableStyle::horzCellMargin() const { return impObj()->horzCellMargin(); }
bool PyDbTableStyle::setHorzCellMargin(double m) { return PyWarnIfError(impObj()->setHorzCellMargin(m), L"AcDbTableStyle::setHorzCellMargin"); }
double PyDbTableStyle::vertCellMargin() const { return impObj()->vertCellMargin(); }
bool PyDbTableStyle::setVertCellMargin(double m) { return PyWarnIfError(impObj()->setVertCellMargin(m), L"AcDbTableStyle::setVertCellMargin"); }
bool PyDbTableStyle::isTitleSuppressed() const { return impObj()->isTitleSuppressed(); }
bool PyDbTableStyle::isHeaderSuppressed() const { return impObj()->isHeaderSuppressed(); }
bool PyDbTableStyle::suppressTitleRow(bool b) { return PyWarnIfError(impObj()->suppressTitleRow(b), L"AcDbTableStyle::suppressTitleRow"); }
bool PyDbTableStyle::suppressHeaderRow(bool b) { return PyWarnIfError(impObj()->suppressHeaderRow(b), L"AcDbTableStyle::suppressHeaderRow"); }
double PyDbTableStyle::margin(int cm, const AcString& cellStyle) const { return impObj()->margin(static_cast<AcDb::CellMargin>(cm), cellStyle.kwszPtr()); }
bool PyDbTableStyle::setMargin(int cm, double fMargin, const AcString& cellStyle) { return PyWarnIfError(impObj()->setMargin(static_cast<AcDb::CellMargin>(cm), fMargin, cellStyle.kwszPtr()), L"AcDbTableStyle::setMargin"); }

void bind_PyDbTableStyle(pym& m) {
	py::class_<PyDbTableStyle, PyDbObject>(m, "PyDbTableStyle")
		.def(py::init<AcDbTableStyle*>(), py::arg("p"))
		.def(py::init([]() { return std::make_unique<PyDbTableStyle>(new AcDbTableStyle()).release(); }), "无参构造（new 默认 AcDbTableStyle）")
		PyWrapper_BIND(DbTableStyle)
		.def("getName", &PyDbTableStyle::getName)
		.def("setName", &PyDbTableStyle::setName, py::arg("name"))
		// isRenamable not exported; skeleton only.
		.def("description", &PyDbTableStyle::description)
		.def("setDescription", &PyDbTableStyle::setDescription, py::arg("desc"))
		.def("bitFlags", &PyDbTableStyle::bitFlags)
		.def("setBitFlags", &PyDbTableStyle::setBitFlags, py::arg("flags"))
		.def("flowDirection", &PyDbTableStyle::flowDirection)
		.def("setFlowDirection", &PyDbTableStyle::setFlowDirection, py::arg("flow"))
		.def("textHeight", &PyDbTableStyle::textHeight)
		.def("setTextHeight", &PyDbTableStyle::setTextHeight, py::arg("h"))
		.def("textStyle", &PyDbTableStyle::textStyle, py::arg("rowType") = 1)
		.def("setTextStyle", &PyDbTableStyle::setTextStyle, py::arg("id"), py::arg("rowTypes") = 7)
		.def("alignment", &PyDbTableStyle::alignment, py::arg("rowType") = 1)
		.def("setAlignment", &PyDbTableStyle::setAlignment, py::arg("align"), py::arg("rowTypes") = 7)
		.def("numCellStyles", &PyDbTableStyle::numCellStyles)
		.def("isCellStyleInUse", &PyDbTableStyle::isCellStyleInUse, py::arg("cellStyle"))
		// ---- 颜色 RGB ----
		.def("color", &PyDbTableStyle::color, py::arg("rowType") = 1, "返回文字色 (r,g,b)")
		.def("setColor", &PyDbTableStyle::setColor, py::arg("r"), py::arg("g"), py::arg("b"), py::arg("rowTypes") = 7)
		.def("backgroundColor", &PyDbTableStyle::backgroundColor, py::arg("rowType") = 1, "返回背景色 (r,g,b)")
		.def("setBackgroundColor", &PyDbTableStyle::setBackgroundColor, py::arg("r"), py::arg("g"), py::arg("b"), py::arg("rowTypes") = 7)
		.def("isBackgroundColorNone", &PyDbTableStyle::isBackgroundColorNone, py::arg("rowType") = 1)
		.def("setBackgroundColorNone", &PyDbTableStyle::setBackgroundColorNone, py::arg("bValue"), py::arg("rowTypes") = 7)
		// ---- 数据类型 / 格式 ----
		.def("getDataType", &PyDbTableStyle::getDataType, "返回 (dataType, unitType)")
		.def("setDataType", &PyDbTableStyle::setDataType, py::arg("dataType"), py::arg("unitType"))
		.def("format", &PyDbTableStyle::format)
		.def("setFormat", &PyDbTableStyle::setFormat, py::arg("fmt"))
		// ---- 网格线属性 ----
		.def("gridLineWeight", &PyDbTableStyle::gridLineWeight, py::arg("gridLineType"), py::arg("rowType") = 1)
		.def("setGridLineWeight", &PyDbTableStyle::setGridLineWeight, py::arg("lineWeight"), py::arg("gridLineTypes") = 63, py::arg("rowTypes") = 7)
		.def("gridColor", &PyDbTableStyle::gridColor, py::arg("gridLineType"), py::arg("rowType") = 1, "返回网格线色 (r,g,b)")
		.def("setGridColor", &PyDbTableStyle::setGridColor, py::arg("r"), py::arg("g"), py::arg("b"), py::arg("gridLineTypes") = 63, py::arg("rowTypes") = 7)
		.def("gridVisibility", &PyDbTableStyle::gridVisibility, py::arg("gridLineType"), py::arg("rowType") = 1)
		.def("setGridVisibility", &PyDbTableStyle::setGridVisibility, py::arg("visible"), py::arg("gridLineTypes") = 63, py::arg("rowTypes") = 7)
		// ---- 边距 / 标题与表头抑制 ----
		.def("horzCellMargin", &PyDbTableStyle::horzCellMargin)
		.def("setHorzCellMargin", &PyDbTableStyle::setHorzCellMargin, py::arg("m"))
		.def("vertCellMargin", &PyDbTableStyle::vertCellMargin)
		.def("setVertCellMargin", &PyDbTableStyle::setVertCellMargin, py::arg("m"))
		.def("isTitleSuppressed", &PyDbTableStyle::isTitleSuppressed)
		.def("isHeaderSuppressed", &PyDbTableStyle::isHeaderSuppressed)
		.def("suppressTitleRow", &PyDbTableStyle::suppressTitleRow, py::arg("b"))
		.def("suppressHeaderRow", &PyDbTableStyle::suppressHeaderRow, py::arg("b"))
		.def("margin", &PyDbTableStyle::margin, py::arg("cellMargin"), py::arg("cellStyle"))
		.def("setMargin", &PyDbTableStyle::setMargin, py::arg("cellMargin"), py::arg("fMargin"), py::arg("cellStyle"));
}
