#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbBody.h"

PyWrapper_Cpp(DbBody)
AcString PyDbBody::toString() const { return AcString(std::format("AcDbBody[wrapper]").c_str(), AcString::Utf8); }
bool PyDbBody::isNull() const { return impObj()->isNull() == Adesk::kTrue; }
unsigned int PyDbBody::numChanges() const { return impObj()->numChanges(); }
void PyDbBody::invalidateCachedBodySubentIds() { impObj()->invalidateCachedBodySubentIds(); }
bool PyDbBody::usesGraphicsCache() const { return impObj()->usesGraphicsCache(); }

std::vector<PyDbBody> PyDbBody::acisIn(const AcString& fileName) {
	AcDbVoidPtrArray out;
	if(!PyWarnIfError(AcDbBody::acisIn(fileName.kwszPtr(), out), L"AcDbBody::acisIn")) return {};
	std::vector<PyDbBody> result;
	result.reserve(out.length());
	for (int i = 0; i < out.length(); ++i) result.emplace_back(static_cast<AcDbBody*>(out[i]));
	return result;
}
bool PyDbBody::acisOut(const AcString& fileName, const std::vector<PyDbBody*>& bodies) {
	AcDbVoidPtrArray in;
	for (auto* b : bodies) if (b) in.append(b->impObj());
	return PyWarnIfError(AcDbBody::acisOut(fileName.kwszPtr(), in), L"AcDbBody::acisOut");
}

void bind_PyDbBody(pym& m) {
	py::class_<PyDbBody, PyDbEntity>(m, "PyDbBody")
		.def(py::init<>())
		.def(py::init<AcDbBody*>(), py::arg("object"))
		.def("isNull", &PyDbBody::isNull)
		.def("numChanges", &PyDbBody::numChanges)
		.def("invalidateCachedBodySubentIds", &PyDbBody::invalidateCachedBodySubentIds)
		.def("usesGraphicsCache", &PyDbBody::usesGraphicsCache)
		.def_static("acisIn", &PyDbBody::acisIn, py::arg("fileName"))
		.def_static("acisOut", &PyDbBody::acisOut, py::arg("fileName"), py::arg("bodies"))
		PyWrapper_BIND(DbBody)
		.def("__repr__", &PyDbBody::toString)
		;
}

