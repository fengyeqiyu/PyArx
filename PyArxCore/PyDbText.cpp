#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbText.h"

PyWrapper_Cpp(DbText)
AcString PyDbText::toString() const { return AcString(std::format("AcDbText[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbText::position() const { return impObj()->position(); }

bool  PyDbText::setPosition(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setPosition(p), L"AcDbText::setPosition"); }

double PyDbText::height() const { return impObj()->height(); }

bool  PyDbText::setHeight(double h) { return PyWarnIfError(impObj()->setHeight(h), L"AcDbText::setHeight"); }

double PyDbText::rotation() const { return impObj()->rotation(); }

bool  PyDbText::setRotation(double r) { return PyWarnIfError(impObj()->setRotation(r), L"AcDbText::setRotation"); }

AcString PyDbText::textString() const { AcString s; if(!PyWarnIfError(impObj()->textString(s), L"AcDbText::textString")) return AcString(); return s; }

bool  PyDbText::setTextString(const AcString& s) { return PyWarnIfError(impObj()->setTextString(s.kwszPtr()), L"AcDbText::setTextString"); }

AcGePoint3d PyDbText::alignmentPoint() const { return impObj()->alignmentPoint(); }
bool  PyDbText::setAlignmentPoint(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setAlignmentPoint(p), L"AcDbText::setAlignmentPoint"); }
bool PyDbText::isDefaultAlignment() const { return impObj()->isDefaultAlignment() != Adesk::kFalse; }
AcGeVector3d PyDbText::normal() const { return impObj()->normal(); }
bool  PyDbText::setNormal(const AcGeVector3d& n) { return PyWarnIfError(impObj()->setNormal(n), L"AcDbText::setNormal"); }
double PyDbText::thickness() const { return impObj()->thickness(); }
bool  PyDbText::setThickness(double v) { return PyWarnIfError(impObj()->setThickness(v), L"AcDbText::setThickness"); }
double PyDbText::oblique() const { return impObj()->oblique(); }
bool  PyDbText::setOblique(double v) { return PyWarnIfError(impObj()->setOblique(v), L"AcDbText::setOblique"); }
double PyDbText::widthFactor() const { return impObj()->widthFactor(); }
bool  PyDbText::setWidthFactor(double v) { return PyWarnIfError(impObj()->setWidthFactor(v), L"AcDbText::setWidthFactor"); }
bool PyDbText::isMirroredInX() const { return impObj()->isMirroredInX() != Adesk::kFalse; }
bool  PyDbText::mirrorInX(bool v) { return PyWarnIfError(impObj()->mirrorInX(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbText::mirrorInX"); }
bool PyDbText::isMirroredInY() const { return impObj()->isMirroredInY() != Adesk::kFalse; }
bool  PyDbText::mirrorInY(bool v) { return PyWarnIfError(impObj()->mirrorInY(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbText::mirrorInY"); }
int PyDbText::horizontalMode() const { return static_cast<int>(impObj()->horizontalMode()); }
bool  PyDbText::setHorizontalMode(int v) { return PyWarnIfError(impObj()->setHorizontalMode(static_cast<AcDb::TextHorzMode>(v)), L"AcDbText::setHorizontalMode"); }
int PyDbText::verticalMode() const { return static_cast<int>(impObj()->verticalMode()); }
bool  PyDbText::setVerticalMode(int v) { return PyWarnIfError(impObj()->setVerticalMode(static_cast<AcDb::TextVertMode>(v)), L"AcDbText::setVerticalMode"); }
int PyDbText::justification() const { return static_cast<int>(impObj()->justification()); }
bool  PyDbText::setJustification(int v) { return PyWarnIfError(impObj()->setJustification(static_cast<AcDbText::AcTextAlignment>(v)), L"AcDbText::setJustification"); }
bool PyDbText::hitTest(const AcGePoint3d& pt) const { return impObj()->hitTest(pt); }
int PyDbText::correctSpelling() { return impObj()->correctSpelling(); }

uint64_t PyDbText::textStyle() const { return static_cast<uint64_t>(impObj()->textStyle().asOldId()); }
bool  PyDbText::setTextStyle(uint64_t id) {
	AcDbObjectId styleId; styleId.setFromOldId(static_cast<Adesk::IntDbId>(id));
	return PyWarnIfError(impObj()->setTextStyle(styleId), L"AcDbText::setTextStyle");
}

bool PyDbText::adjustAlignment() { return PyWarnIfError(impObj()->adjustAlignment(nullptr), L"AcDbText::adjustAlignment"); }

std::vector<AcGePoint3d> PyDbText::getBoundingPoints() const {
	AcGePoint3dArray arr;
	impObj()->getBoundingPoints(arr);
	std::vector<AcGePoint3d> out;
	out.reserve(arr.length());
	for (int i = 0; i < arr.length(); ++i) out.push_back(arr[i]);
	return out;
}

bool PyDbText::convertFieldToText() { return PyWarnIfError(impObj()->convertFieldToText(), L"AcDbText::convertFieldToText"); }

py::tuple PyDbText::getPlane() const {
	AcGePlane plane; AcDb::Planarity type;
	if (!PyWarnIfError(impObj()->getPlane(plane, type), L"AcDbText::getPlane"))
		return py::make_tuple(AcGePlane(), 0);
	return py::make_tuple(plane, static_cast<int>(type));
}

bool PyDbText::isPlanar() const { return impObj()->isPlanar() != Adesk::kFalse; }

bool PyDbText::removeField(const AcString& propName) { return PyWarnIfError(impObj()->removeField(propName.kwszPtr()), L"AcDbText::removeField"); }
bool PyDbText::removeField(uint64_t fieldId) {
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::IntDbId>(fieldId));
	return PyWarnIfError(impObj()->removeField(id), L"AcDbText::removeField");
}

void bind_PyDbText(pym& m) {
	py::class_<PyDbText, PyDbEntity>(m, "PyDbText")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, const AcString&, double, double>(),
			py::arg("position"), py::arg("text"), py::arg("height") = 0.0, py::arg("rotation") = 0.0)
		.def(py::init<AcDbText*>(), py::arg("object"))
		.def("position", &PyDbText::position)
		.def("setPosition", &PyDbText::setPosition, py::arg("p"))
		.def("alignmentPoint", &PyDbText::alignmentPoint)
		.def("setAlignmentPoint", &PyDbText::setAlignmentPoint, py::arg("p"))
		.def("isDefaultAlignment", &PyDbText::isDefaultAlignment)
		.def("normal", &PyDbText::normal)
		.def("setNormal", &PyDbText::setNormal, py::arg("n"))
		.def("thickness", &PyDbText::thickness)
		.def("setThickness", &PyDbText::setThickness, py::arg("t"))
		.def("oblique", &PyDbText::oblique)
		.def("setOblique", &PyDbText::setOblique, py::arg("v"))
		.def("height", &PyDbText::height)
		.def("setHeight", &PyDbText::setHeight, py::arg("h"))
		.def("rotation", &PyDbText::rotation)
		.def("setRotation", &PyDbText::setRotation, py::arg("r"))
		.def("widthFactor", &PyDbText::widthFactor)
		.def("setWidthFactor", &PyDbText::setWidthFactor, py::arg("v"))
		.def("textString", &PyDbText::textString)
		.def("setTextString", &PyDbText::setTextString, py::arg("text"))
		.def("isMirroredInX", &PyDbText::isMirroredInX)
		.def("mirrorInX", &PyDbText::mirrorInX, py::arg("v"))
		.def("isMirroredInY", &PyDbText::isMirroredInY)
		.def("mirrorInY", &PyDbText::mirrorInY, py::arg("v"))
		.def("horizontalMode", &PyDbText::horizontalMode)
		.def("setHorizontalMode", &PyDbText::setHorizontalMode, py::arg("v"))
		.def("verticalMode", &PyDbText::verticalMode)
		.def("setVerticalMode", &PyDbText::setVerticalMode, py::arg("v"))
		.def("justification", &PyDbText::justification)
		.def("setJustification", &PyDbText::setJustification, py::arg("v"))
		.def("hitTest", &PyDbText::hitTest, py::arg("pt"))
		.def("correctSpelling", &PyDbText::correctSpelling)
		.def("textStyle", &PyDbText::textStyle)
		.def("setTextStyle", &PyDbText::setTextStyle, py::arg("styleId"))
		.def("adjustAlignment", &PyDbText::adjustAlignment)
		.def("getBoundingPoints", &PyDbText::getBoundingPoints)
		.def("convertFieldToText", &PyDbText::convertFieldToText)
		.def("getPlane", &PyDbText::getPlane)
		.def("isPlanar", &PyDbText::isPlanar)
		.def("removeField", py::overload_cast<const AcString&>(&PyDbText::removeField), py::arg("propName"))
		.def("removeField", py::overload_cast<uint64_t>(&PyDbText::removeField), py::arg("fieldId"))
		PyWrapper_BIND(DbText)
		.def("__repr__", &PyDbText::toString)
		;
}

