#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyAcadDatabase.h"
#include "PyDbDatabase.h"

namespace {
	AcDbDatabase* workingDb() {
		auto* hs = acdbHostApplicationServices();
		return hs ? hs->workingDatabase() : nullptr;
	}
}

PyDbDatabase PyAcadDatabase::current() {
	return PyDbDatabase(workingDb(), false);
}

bool PyAcadDatabase::isAvailable() {
	return workingDb() != nullptr;
}

AcString PyAcadDatabase::toString() const {
	return AcString(std::format("PyAcadDatabase[working={}]", isAvailable() ? "yes" : "no").c_str(), AcString::Utf8);
}

void bind_PyAcadDatabase(pym& m) {
	py::class_<PyAcadDatabase>(m, "PyAcadDatabase")
		.def_static("current", &PyAcadDatabase::current)
		.def_static("isAvailable", &PyAcadDatabase::isAvailable)
		.def("__repr__", &PyAcadDatabase::toString);
}
