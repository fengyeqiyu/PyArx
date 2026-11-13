#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbHyperlinkCollection.h"

int PyDbHyperlinkCollection::count() const {
	return impObj() ? impObj()->count() : 0;
}

bool PyDbHyperlinkCollection::addHead(const AcString& name, const AcString& desc, const AcString& subLoc) {
	if (!impObj()) return false;
	impObj()->addHead(name.kwszPtr(), desc.kwszPtr(), subLoc.kwszPtr());
	return true;
}

bool PyDbHyperlinkCollection::addTail(const AcString& name, const AcString& desc, const AcString& subLoc) {
	if (!impObj()) return false;
	impObj()->addTail(name.kwszPtr(), desc.kwszPtr(), subLoc.kwszPtr());
	return true;
}

bool PyDbHyperlinkCollection::addAt(int index, const AcString& name, const AcString& desc, const AcString& subLoc) {
	if (!impObj()) return false;
	impObj()->addAt(index, name.kwszPtr(), desc.kwszPtr(), subLoc.kwszPtr());
	return true;
}

bool PyDbHyperlinkCollection::removeHead() {
	if (!impObj()) return false;
	impObj()->removeHead();
	return true;
}

bool PyDbHyperlinkCollection::removeTail() {
	if (!impObj()) return false;
	impObj()->removeTail();
	return true;
}

bool PyDbHyperlinkCollection::removeAt(int index) {
	if (!impObj()) return false;
	impObj()->removeAt(index);
	return true;
}

PyDbHyperlink PyDbHyperlinkCollection::item(int index) const {
	if (!impObj() || index < 0 || index >= impObj()->count()) return PyDbHyperlink(nullptr);
	return PyDbHyperlink(impObj()->item(index));
}

AcString PyDbHyperlinkCollection::toString() const {
	return AcString(std::format("PyDbHyperlinkCollection[count={}]", count()).c_str(), AcString::Utf8);
}

void bind_PyDbHyperlinkCollection(pym& m) {
	py::class_<PyDbHyperlinkCollection>(m, "PyDbHyperlinkCollection")
		.def(py::init<AcDbHyperlinkCollection*>(), py::arg("object"))
		.def("count", &PyDbHyperlinkCollection::count)
		.def("addHead", &PyDbHyperlinkCollection::addHead,
			py::arg("name"), py::arg("desc"), py::arg("subLoc") = L"")
		.def("addTail", &PyDbHyperlinkCollection::addTail,
			py::arg("name"), py::arg("desc"), py::arg("subLoc") = L"")
		.def("addAt", &PyDbHyperlinkCollection::addAt,
			py::arg("index"), py::arg("name"), py::arg("desc"), py::arg("subLoc") = L"")
		.def("removeHead", &PyDbHyperlinkCollection::removeHead)
		.def("removeTail", &PyDbHyperlinkCollection::removeTail)
		.def("removeAt", &PyDbHyperlinkCollection::removeAt, py::arg("index"))
		.def("item", &PyDbHyperlinkCollection::item, py::arg("index"))
		.def("__repr__", &PyDbHyperlinkCollection::toString);
}
