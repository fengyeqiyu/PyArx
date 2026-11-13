#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbObjectContextManager.h"

bool PyDbObjectContextManager::unregisterContextCollection(const AcString& collectionName) {
	if (!m_ptr) return false;
	return PyWarnIfError(m_ptr->unregisterContextCollection(collectionName.kwszPtr()),
		L"ObjectContextManager.unregisterContextCollection");
}

AcString PyDbObjectContextManager::toString() const {
	return AcString(std::format("PyDbObjectContextManager[{}]", m_ptr ? "valid" : "null").c_str(), AcString::Utf8);
}

void bind_PyDbObjectContextManager(pym& m) {
	py::class_<PyDbObjectContextManager>(m, "PyDbObjectContextManager")
		.def(py::init<AcDbObjectContextManager*>(), py::arg("object") = nullptr)
		.def("isNull", &PyDbObjectContextManager::isNull)
		.def("unregisterContextCollection", &PyDbObjectContextManager::unregisterContextCollection,
			py::arg("collectionName"))
		.def("__repr__", &PyDbObjectContextManager::toString);
}
