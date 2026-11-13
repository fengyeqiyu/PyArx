#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbMlineStyle.h"

PyWrapper_Cpp(DbMlineStyle);

AcString PyDbMlineStyle::toString() const { return AcString(std::format("AcDbMlineStyle[wrapper]").c_str(), AcString::Utf8); }

AcString PyDbMlineStyle::description() const { const ACHAR* p = impObj()->description(); return p ? AcString(p) : AcString(); }
bool PyDbMlineStyle::setDescription(const AcString& desc) { return PyWarnIfError(impObj()->setDescription(desc.kwszPtr()), L"AcDbMlineStyle::setDescription"); }
AcString PyDbMlineStyle::styleName() const { const ACHAR* p = impObj()->name(); return p ? AcString(p) : AcString(); }
bool PyDbMlineStyle::setStyleName(const AcString& name) { return PyWarnIfError(impObj()->setName(name.kwszPtr()), L"AcDbMlineStyle::setName"); }

bool PyDbMlineStyle::showMiters() const { return impObj()->showMiters(); }
bool PyDbMlineStyle::setShowMiters(bool show) { impObj()->setShowMiters(show); return true; }
bool PyDbMlineStyle::startSquareCap() const { return impObj()->startSquareCap(); }
bool PyDbMlineStyle::setStartSquareCap(bool show) { impObj()->setStartSquareCap(show); return true; }
bool PyDbMlineStyle::startRoundCap() const { return impObj()->startRoundCap(); }
bool PyDbMlineStyle::setStartRoundCap(bool show) { impObj()->setStartRoundCap(show); return true; }
bool PyDbMlineStyle::startInnerArcs() const { return impObj()->startInnerArcs(); }
bool PyDbMlineStyle::setStartInnerArcs(bool show) { impObj()->setStartInnerArcs(show); return true; }
bool PyDbMlineStyle::endSquareCap() const { return impObj()->endSquareCap(); }
bool PyDbMlineStyle::setEndSquareCap(bool show) { impObj()->setEndSquareCap(show); return true; }
bool PyDbMlineStyle::endRoundCap() const { return impObj()->endRoundCap(); }
bool PyDbMlineStyle::setEndRoundCap(bool show) { impObj()->setEndRoundCap(show); return true; }
bool PyDbMlineStyle::endInnerArcs() const { return impObj()->endInnerArcs(); }
bool PyDbMlineStyle::setEndInnerArcs(bool show) { impObj()->setEndInnerArcs(show); return true; }
bool PyDbMlineStyle::filled() const { return impObj()->filled(); }
bool PyDbMlineStyle::setFilled(bool onOff) { impObj()->setFilled(onOff); return true; }

double PyDbMlineStyle::startAngle() const { return impObj()->startAngle(); }
bool PyDbMlineStyle::setStartAngle(double ang) { return PyWarnIfError(impObj()->setStartAngle(ang), L"AcDbMlineStyle::setStartAngle"); }
double PyDbMlineStyle::endAngle() const { return impObj()->endAngle(); }
bool PyDbMlineStyle::setEndAngle(double ang) { return PyWarnIfError(impObj()->setEndAngle(ang), L"AcDbMlineStyle::setEndAngle"); }
int PyDbMlineStyle::numElements() const { return impObj()->numElements(); }

int PyDbMlineStyle::addElement(double offset, int r, int g, int b, uint64_t linetypeId, bool checkIfReferenced) {
	int idx = 0;
	AcCmColor c; c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
	AcDbObjectId ltId; ltId.setFromOldId(static_cast<Adesk::IntDbId>(linetypeId));
	if (!PyWarnIfError(impObj()->addElement(idx, offset, c, ltId, checkIfReferenced), L"AcDbMlineStyle::addElement"))
		return -1;
	return idx;
}
std::tuple<double, int, int, int, uint64_t> PyDbMlineStyle::getElementAt(int elem) const {
	double offset = 0.0; AcCmColor c; AcDbObjectId ltId;
	if (!PyWarnIfError(impObj()->getElementAt(elem, offset, c, ltId), L"AcDbMlineStyle::getElementAt"))
		return std::make_tuple(0.0, 0, 0, 0, 0ULL);
	return std::make_tuple(offset, (int)c.red(), (int)c.green(), (int)c.blue(),
		ltId.isNull() ? 0ULL : static_cast<uint64_t>(ltId.asOldId()));
}
bool PyDbMlineStyle::setElement(int elem, double offset, int r, int g, int b, uint64_t linetypeId) {
	AcCmColor c; c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
	AcDbObjectId ltId; ltId.setFromOldId(static_cast<Adesk::IntDbId>(linetypeId));
	int e = elem;
	return PyWarnIfError(impObj()->setElement(e, offset, c, ltId), L"AcDbMlineStyle::setElement");
}
bool PyDbMlineStyle::removeElementAt(int elem) { return PyWarnIfError(impObj()->removeElementAt(elem), L"AcDbMlineStyle::removeElementAt"); }
bool PyDbMlineStyle::setFillColor(int r, int g, int b) {
	AcCmColor c; c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
	return PyWarnIfError(impObj()->setFillColor(c), L"AcDbMlineStyle::setFillColor");
}
std::tuple<int, int, int> PyDbMlineStyle::fillColor() const {
	const AcCmColor& c = impObj()->fillColor();
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
void PyDbMlineStyle::initMlineStyle() { impObj()->initMlineStyle(); }
bool PyDbMlineStyle::setFrom(const PyDbMlineStyle& src, bool checkIfReferenced) {
	impObj()->set(*src.impObj(), checkIfReferenced);
	return true;
}

void bind_PyDbMlineStyle(pym& m) {
	py::class_<PyDbMlineStyle, PyDbObject>(m, "PyDbMlineStyle")
		.def(py::init<>())
		.def(py::init<AcDbMlineStyle*>(), py::arg("p"))
		PyWrapper_BIND(DbMlineStyle)
		.def("description", &PyDbMlineStyle::description)
		.def("setDescription", &PyDbMlineStyle::setDescription, py::arg("desc"))
		.def("styleName", &PyDbMlineStyle::styleName)
		.def("setStyleName", &PyDbMlineStyle::setStyleName, py::arg("name"))
		.def("showMiters", &PyDbMlineStyle::showMiters)
		.def("setShowMiters", &PyDbMlineStyle::setShowMiters, py::arg("show"))
		.def("startSquareCap", &PyDbMlineStyle::startSquareCap)
		.def("setStartSquareCap", &PyDbMlineStyle::setStartSquareCap, py::arg("show"))
		.def("startRoundCap", &PyDbMlineStyle::startRoundCap)
		.def("setStartRoundCap", &PyDbMlineStyle::setStartRoundCap, py::arg("show"))
		.def("startInnerArcs", &PyDbMlineStyle::startInnerArcs)
		.def("setStartInnerArcs", &PyDbMlineStyle::setStartInnerArcs, py::arg("show"))
		.def("endSquareCap", &PyDbMlineStyle::endSquareCap)
		.def("setEndSquareCap", &PyDbMlineStyle::setEndSquareCap, py::arg("show"))
		.def("endRoundCap", &PyDbMlineStyle::endRoundCap)
		.def("setEndRoundCap", &PyDbMlineStyle::setEndRoundCap, py::arg("show"))
		.def("endInnerArcs", &PyDbMlineStyle::endInnerArcs)
		.def("setEndInnerArcs", &PyDbMlineStyle::setEndInnerArcs, py::arg("show"))
		.def("filled", &PyDbMlineStyle::filled)
		.def("setFilled", &PyDbMlineStyle::setFilled, py::arg("onOff"))
		.def("startAngle", &PyDbMlineStyle::startAngle)
		.def("setStartAngle", &PyDbMlineStyle::setStartAngle, py::arg("ang"))
		.def("endAngle", &PyDbMlineStyle::endAngle)
		.def("setEndAngle", &PyDbMlineStyle::setEndAngle, py::arg("ang"))
		.def("numElements", &PyDbMlineStyle::numElements)
		.def("addElement", &PyDbMlineStyle::addElement,
			py::arg("offset"), py::arg("r"), py::arg("g"), py::arg("b"), py::arg("linetypeId"), py::arg("checkIfReferenced") = true,
			"添加一个线元素，返回新元素索引；失败返回 -1")
		.def("getElementAt", &PyDbMlineStyle::getElementAt, py::arg("elem"),
			"返回 (offset, r, g, b, linetypeId)；失败返回全 0")
		.def("setElement", &PyDbMlineStyle::setElement,
			py::arg("elem"), py::arg("offset"), py::arg("r"), py::arg("g"), py::arg("b"), py::arg("linetypeId"))
		.def("removeElementAt", &PyDbMlineStyle::removeElementAt, py::arg("elem"))
		.def("setFillColor", &PyDbMlineStyle::setFillColor, py::arg("r"), py::arg("g"), py::arg("b"))
		.def("fillColor", &PyDbMlineStyle::fillColor, "返回 (r, g, b)")
		.def("initMlineStyle", &PyDbMlineStyle::initMlineStyle, "重置为默认样式")
		.def("setFrom", &PyDbMlineStyle::setFrom, py::arg("src"), py::arg("checkIfReferenced") = true,
			"从另一个 MlineStyle 复制参数")
		.def("__repr__", &PyDbMlineStyle::toString)
;
}
