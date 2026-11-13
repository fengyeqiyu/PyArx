#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbHyperlink.h"

namespace {
	AcString achToW(const ACHAR* p) {
		return p ? AcString(p) : AcString();
	}
}

AcString PyDbHyperlink::name() const {
	return impObj() ? achToW(impObj()->name()) : AcString();
}
bool PyDbHyperlink::setName(const AcString& v) {
	if (!impObj()) return false;
	impObj()->setName(v.kwszPtr());
	return true;
}

AcString PyDbHyperlink::description() const {
	return impObj() ? achToW(impObj()->description()) : AcString();
}
bool PyDbHyperlink::setDescription(const AcString& v) {
	if (!impObj()) return false;
	impObj()->setDescription(v.kwszPtr());
	return true;
}

AcString PyDbHyperlink::subLocation() const {
	return impObj() ? achToW(impObj()->subLocation()) : AcString();
}
bool PyDbHyperlink::setSubLocation(const AcString& v) {
	if (!impObj()) return false;
	impObj()->setSubLocation(v.kwszPtr());
	return true;
}

AcString PyDbHyperlink::displayString() const {
	return impObj() ? achToW(impObj()->getDisplayString()) : AcString();
}

bool PyDbHyperlink::isOutermostContainer() const {
	return impObj() && impObj()->isOutermostContainer();
}

int PyDbHyperlink::nestedLevel() const {
	return impObj() ? impObj()->getNestedLevel() : 0;
}

int PyDbHyperlink::flags() const {
	return impObj() ? static_cast<int>(impObj()->flags()) : 0;
}
bool PyDbHyperlink::setFlags(int f) {
	if (!impObj()) return false;
	impObj()->setFlags(static_cast<Adesk::Int32>(f));
	return true;
}

AcString PyDbHyperlink::toString() const {
	return AcString(std::format("PyDbHyperlink[nestedLevel={},outer={},flags={}]",
		nestedLevel(), isOutermostContainer(), flags()).c_str(), AcString::Utf8);
}

void bind_PyDbHyperlink(pym& m) {
	py::class_<PyDbHyperlink>(m, "PyDbHyperlink")
		.def(py::init<AcDbHyperlink*>(), py::arg("object"))
		.def("name", &PyDbHyperlink::name)
		.def("setName", &PyDbHyperlink::setName, py::arg("v"))
		.def("description", &PyDbHyperlink::description)
		.def("setDescription", &PyDbHyperlink::setDescription, py::arg("v"))
		.def("subLocation", &PyDbHyperlink::subLocation)
		.def("setSubLocation", &PyDbHyperlink::setSubLocation, py::arg("v"))
		.def("displayString", &PyDbHyperlink::displayString)
		.def("isOutermostContainer", &PyDbHyperlink::isOutermostContainer)
		.def("nestedLevel", &PyDbHyperlink::nestedLevel)
		.def("flags", &PyDbHyperlink::flags)
		.def("setFlags", &PyDbHyperlink::setFlags, py::arg("f"))
		.def("__repr__", &PyDbHyperlink::toString);
}
