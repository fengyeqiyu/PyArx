#include "stdafx.h"
#include "PyDbDimension.h"
#include "PyErrorStatusException.h"

PyWrapper_Cpp(DbDimension)

AcString PyDbDimension::toString() const { return AcString(std::format("AcDbDimension[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbDimension::textPosition() const { if (!impObj()) return AcGePoint3d(); return impObj()->textPosition(); }
bool  PyDbDimension::setTextPosition(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setTextPosition(p), L"AcDbDimension::setTextPosition"); }
AcString PyDbDimension::dimensionText() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->dimensionText(s), L"AcDbDimension::dimensionText")) return AcString(); return s; }
bool  PyDbDimension::setDimensionText(const AcString& s) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimensionText(s.kwszPtr()), L"AcDbDimension::setDimensionText"); }
AcGeVector3d PyDbDimension::normal() const { if (!impObj()) return AcGeVector3d(); return impObj()->normal(); }
bool  PyDbDimension::setNormal(const AcGeVector3d& n) { if (!impObj()) return false; return PyWarnIfError(impObj()->setNormal(n), L"AcDbDimension::setNormal"); }
double PyDbDimension::elevation() const { if (!impObj()) return 0.0; return impObj()->elevation(); }
bool  PyDbDimension::setElevation(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setElevation(v), L"AcDbDimension::setElevation"); }
double PyDbDimension::textRotation() const { if (!impObj()) return 0.0; return impObj()->textRotation(); }
bool  PyDbDimension::setTextRotation(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setTextRotation(v), L"AcDbDimension::setTextRotation"); }
double PyDbDimension::horizontalRotation() const { if (!impObj()) return 0.0; return impObj()->horizontalRotation(); }
bool  PyDbDimension::setHorizontalRotation(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setHorizontalRotation(v), L"AcDbDimension::setHorizontalRotation"); }
double PyDbDimension::measurement() { if (!impObj()) return 0.0; double v = 0.0; if(!PyWarnIfError(impObj()->measurement(v), L"AcDbDimension::measurement")) return 0.0; return v; }
bool PyDbDimension::isUsingDefaultTextPosition() const { if (!impObj()) return false; return impObj()->isUsingDefaultTextPosition() != Adesk::kFalse; }
bool  PyDbDimension::useSetTextPosition() { if (!impObj()) return false; return PyWarnIfError(impObj()->useSetTextPosition(), L"AcDbDimension::useSetTextPosition"); }
bool  PyDbDimension::useDefaultTextPosition() { if (!impObj()) return false; return PyWarnIfError(impObj()->useDefaultTextPosition(), L"AcDbDimension::useDefaultTextPosition"); }
bool  PyDbDimension::recomputeDimBlock(bool forceUpdate) { if (!impObj()) return false; return PyWarnIfError(impObj()->recomputeDimBlock(forceUpdate), L"AcDbDimension::recomputeDimBlock"); }
bool  PyDbDimension::generateLayout() { if (!impObj()) return false; return PyWarnIfError(impObj()->generateLayout(), L"AcDbDimension::generateLayout"); }
bool PyDbDimension::isDynamicDimension() const { if (!impObj()) return false; return impObj()->isDynamicDimension(); }
bool  PyDbDimension::setDynamicDimension(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDynamicDimension(v), L"AcDbDimension::setDynamicDimension"); }

// ---- 标注样式 ID ----
uint64_t PyDbDimension::dimensionStyle() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->dimensionStyle();
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
bool PyDbDimension::setDimensionStyle(uint64_t styleId) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::IntDbId>(styleId));
	return PyWarnIfError(impObj()->setDimensionStyle(id), L"AcDbDimension::setDimensionStyle");
}

// ---- 默认文字位置开关 ----
bool PyDbDimension::setUsingDefaultTextPosition(bool v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setUsingDefaultTextPosition(v), L"AcDbDimension::setUsingDefaultTextPosition");
}

// ---- 文字附着方式（AcDbMText::AttachmentPoint，int 透出）----
int  PyDbDimension::textAttachment() const { if (!impObj()) return 0; return static_cast<int>(impObj()->textAttachment()); }
bool PyDbDimension::setTextAttachment(int v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setTextAttachment(static_cast<AcDbMText::AttachmentPoint>(v)), L"AcDbDimension::setTextAttachment");
}

// ---- 前缀 / 后缀 ----
AcString PyDbDimension::prefix() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->prefix(s), L"AcDbDimension::prefix")) return AcString(); return s; }
bool PyDbDimension::setPrefix(const AcString& s) { if (!impObj()) return false; return PyWarnIfError(impObj()->setPrefix(s), L"AcDbDimension::setPrefix"); }
AcString PyDbDimension::suffix() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->suffix(s), L"AcDbDimension::suffix")) return AcString(); return s; }
bool PyDbDimension::setSuffix(const AcString& s) { if (!impObj()) return false; return PyWarnIfError(impObj()->setSuffix(s), L"AcDbDimension::setSuffix"); }

// ---- 箭头翻转 ----
bool PyDbDimension::getArrowFirstIsFlipped() const { if (!impObj()) return false; return impObj()->getArrowFirstIsFlipped(); }
bool PyDbDimension::getArrowSecondIsFlipped() const { if (!impObj()) return false; return impObj()->getArrowSecondIsFlipped(); }
bool PyDbDimension::setArrowFirstIsFlipped(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setArrowFirstIsFlipped(v), L"AcDbDimension::setArrowFirstIsFlipped"); }
bool PyDbDimension::setArrowSecondIsFlipped(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setArrowSecondIsFlipped(v), L"AcDbDimension::setArrowSecondIsFlipped"); }

// ---- 高频标注变量覆写（dbdimvar.h）----
double PyDbDimension::dimtxt() const { if (!impObj()) return 0.0; return impObj()->dimtxt(); }
bool   PyDbDimension::setDimtxt(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimtxt(v), L"AcDbDimension::setDimtxt"); }
double PyDbDimension::dimgap() const { if (!impObj()) return 0.0; return impObj()->dimgap(); }
bool   PyDbDimension::setDimgap(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimgap(v), L"AcDbDimension::setDimgap"); }
double PyDbDimension::dimasz() const { if (!impObj()) return 0.0; return impObj()->dimasz(); }
bool   PyDbDimension::setDimasz(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimasz(v), L"AcDbDimension::setDimasz"); }
int    PyDbDimension::dimdec() const { if (!impObj()) return 0; return impObj()->dimdec(); }
bool   PyDbDimension::setDimdec(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimdec(v), L"AcDbDimension::setDimdec"); }
bool   PyDbDimension::dimtol() const { if (!impObj()) return false; return impObj()->dimtol(); }
bool   PyDbDimension::setDimtol(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimtol(v), L"AcDbDimension::setDimtol"); }
double PyDbDimension::dimtp() const { if (!impObj()) return 0.0; return impObj()->dimtp(); }
bool   PyDbDimension::setDimtp(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimtp(v), L"AcDbDimension::setDimtp"); }
double PyDbDimension::dimtm() const { if (!impObj()) return 0.0; return impObj()->dimtm(); }
bool   PyDbDimension::setDimtm(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimtm(v), L"AcDbDimension::setDimtm"); }
double PyDbDimension::dimscale() const { if (!impObj()) return 0.0; return impObj()->dimscale(); }
bool   PyDbDimension::setDimscale(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimscale(v), L"AcDbDimension::setDimscale"); }
uint64_t PyDbDimension::dimtxsty() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->dimtxsty();
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
bool   PyDbDimension::setDimtxsty(uint64_t idv) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::IntDbId>(idv));
	return PyWarnIfError(impObj()->setDimtxsty(id), L"AcDbDimension::setDimtxsty");
}
bool   PyDbDimension::dimse1() const { if (!impObj()) return false; return impObj()->dimse1(); }
bool   PyDbDimension::setDimse1(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimse1(v), L"AcDbDimension::setDimse1"); }
bool   PyDbDimension::dimse2() const { if (!impObj()) return false; return impObj()->dimse2(); }
bool   PyDbDimension::setDimse2(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimse2(v), L"AcDbDimension::setDimse2"); }
double PyDbDimension::dimexe() const { if (!impObj()) return 0.0; return impObj()->dimexe(); }
bool   PyDbDimension::setDimexe(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimexe(v), L"AcDbDimension::setDimexe"); }
double PyDbDimension::dimexo() const { if (!impObj()) return 0.0; return impObj()->dimexo(); }
bool   PyDbDimension::setDimexo(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimexo(v), L"AcDbDimension::setDimexo"); }
int    PyDbDimension::dimtad() const { if (!impObj()) return 0; return impObj()->dimtad(); }
bool   PyDbDimension::setDimtad(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDimtad(v), L"AcDbDimension::setDimtad"); }

void bind_PyDbDimension(pym& m) {
	py::class_<PyDbDimension, PyDbEntity>(m, "PyDbDimension")
		.def(py::init<AcDbDimension*>(), py::arg("object"))
		.def("textPosition", &PyDbDimension::textPosition)
		.def("setTextPosition", &PyDbDimension::setTextPosition, py::arg("p"))
		.def("dimensionText", &PyDbDimension::dimensionText)
		.def("setDimensionText", &PyDbDimension::setDimensionText, py::arg("text"))
		.def("normal", &PyDbDimension::normal)
		.def("setNormal", &PyDbDimension::setNormal, py::arg("n"))
		.def("elevation", &PyDbDimension::elevation)
		.def("setElevation", &PyDbDimension::setElevation, py::arg("v"))
		.def("textRotation", &PyDbDimension::textRotation)
		.def("setTextRotation", &PyDbDimension::setTextRotation, py::arg("v"))
		.def("horizontalRotation", &PyDbDimension::horizontalRotation)
		.def("setHorizontalRotation", &PyDbDimension::setHorizontalRotation, py::arg("v"))
		.def("measurement", &PyDbDimension::measurement)
		.def("isUsingDefaultTextPosition", &PyDbDimension::isUsingDefaultTextPosition)
		.def("useSetTextPosition", &PyDbDimension::useSetTextPosition)
		.def("useDefaultTextPosition", &PyDbDimension::useDefaultTextPosition)
		.def("recomputeDimBlock", &PyDbDimension::recomputeDimBlock, py::arg("forceUpdate") = true)
		.def("generateLayout", &PyDbDimension::generateLayout)
		.def("isDynamicDimension", &PyDbDimension::isDynamicDimension)
		.def("setDynamicDimension", &PyDbDimension::setDynamicDimension, py::arg("v"))
		// ---- 标注样式 ID ----
		.def("dimensionStyle", &PyDbDimension::dimensionStyle, "标注样式 ObjectId（0 表示空）")
		.def("setDimensionStyle", &PyDbDimension::setDimensionStyle, py::arg("styleId"), "设置标注样式 ObjectId")
		// ---- 默认文字位置开关 ----
		.def("setUsingDefaultTextPosition", &PyDbDimension::setUsingDefaultTextPosition, py::arg("v"), "是否使用默认文字位置")
		// ---- 文字附着方式 ----
		.def("textAttachment", &PyDbDimension::textAttachment, "文字附着方式（AcDbMText::AttachmentPoint）")
		.def("setTextAttachment", &PyDbDimension::setTextAttachment, py::arg("v"), "设置文字附着方式")
		// ---- 前缀 / 后缀 ----
		.def("prefix", &PyDbDimension::prefix)
		.def("setPrefix", &PyDbDimension::setPrefix, py::arg("text"))
		.def("suffix", &PyDbDimension::suffix)
		.def("setSuffix", &PyDbDimension::setSuffix, py::arg("text"))
		// ---- 箭头翻转 ----
		.def("getArrowFirstIsFlipped", &PyDbDimension::getArrowFirstIsFlipped)
		.def("getArrowSecondIsFlipped", &PyDbDimension::getArrowSecondIsFlipped)
		.def("setArrowFirstIsFlipped", &PyDbDimension::setArrowFirstIsFlipped, py::arg("v"))
		.def("setArrowSecondIsFlipped", &PyDbDimension::setArrowSecondIsFlipped, py::arg("v"))
		// ---- 高频标注变量覆写 ----
		.def("dimtxt", &PyDbDimension::dimtxt)
		.def("setDimtxt", &PyDbDimension::setDimtxt, py::arg("v"))
		.def("dimgap", &PyDbDimension::dimgap)
		.def("setDimgap", &PyDbDimension::setDimgap, py::arg("v"))
		.def("dimasz", &PyDbDimension::dimasz)
		.def("setDimasz", &PyDbDimension::setDimasz, py::arg("v"))
		.def("dimdec", &PyDbDimension::dimdec)
		.def("setDimdec", &PyDbDimension::setDimdec, py::arg("v"))
		.def("dimtol", &PyDbDimension::dimtol)
		.def("setDimtol", &PyDbDimension::setDimtol, py::arg("v"))
		.def("dimtp", &PyDbDimension::dimtp)
		.def("setDimtp", &PyDbDimension::setDimtp, py::arg("v"))
		.def("dimtm", &PyDbDimension::dimtm)
		.def("setDimtm", &PyDbDimension::setDimtm, py::arg("v"))
		.def("dimscale", &PyDbDimension::dimscale)
		.def("setDimscale", &PyDbDimension::setDimscale, py::arg("v"))
		.def("dimtxsty", &PyDbDimension::dimtxsty, "标注文字样式 ObjectId（0 表示空）")
		.def("setDimtxsty", &PyDbDimension::setDimtxsty, py::arg("id"))
		.def("dimse1", &PyDbDimension::dimse1)
		.def("setDimse1", &PyDbDimension::setDimse1, py::arg("v"))
		.def("dimse2", &PyDbDimension::dimse2)
		.def("setDimse2", &PyDbDimension::setDimse2, py::arg("v"))
		.def("dimexe", &PyDbDimension::dimexe)
		.def("setDimexe", &PyDbDimension::setDimexe, py::arg("v"))
		.def("dimexo", &PyDbDimension::dimexo)
		.def("setDimexo", &PyDbDimension::setDimexo, py::arg("v"))
		.def("dimtad", &PyDbDimension::dimtad)
		.def("setDimtad", &PyDbDimension::setDimtad, py::arg("v"))
		PyWrapper_BIND(DbDimension)
		.def("__repr__", &PyDbDimension::toString)
		;
}
