#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyUtils.h"
#include "PyDbLayout.h"

PyWrapper_Cpp(DbLayout);

uint64_t PyDbLayout::getBlockTableRecordId() const {
	return static_cast<uint64_t>(impObj()->getBlockTableRecordId().asOldId());
}

bool PyDbLayout::setBlockTableRecordId(uint64_t btrId) {
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(btrId));
	return PyWarnIfError(impObj()->setBlockTableRecordId(id), L"Layout.setBlockTableRecordId");
}

py::list PyDbLayout::getViewportArray() const {
	return py::cast(impObj()->getViewportArray());
}

py::tuple PyDbLayout::getLimits() const {
	AcGePoint2d limMin, limMax;
	impObj()->getLimits(limMin, limMax);
	return py::make_tuple(
		py::make_tuple(limMin.x, limMin.y),
		py::make_tuple(limMax.x, limMax.y));
}

py::tuple PyDbLayout::getExtents() const {
	AcGePoint3d extMin, extMax;
	impObj()->getExtents(extMin, extMax);
	return py::make_tuple(
		py::make_tuple(extMin.x, extMin.y, extMin.z),
		py::make_tuple(extMax.x, extMax.y, extMax.z));
}

bool PyDbLayout::annoAllVisible() const {
	return impObj()->annoAllVisible();
}

bool PyDbLayout::setAnnoAllVisible(bool visible) {
	return PyWarnIfError(impObj()->setAnnoAllVisible(visible), L"Layout.setAnnoAllVisible");
}

AcString PyDbLayout::getLayoutName() const {
	AcString s;
	if(!PyWarnIfError(impObj()->getName(s), L"Layout.getName")) return AcString();
	return s;
}

bool  PyDbLayout::setLayoutName(const AcString& name) {
	return PyWarnIfError(impObj()->setLayoutName(name.kwszPtr()), L"Layout.setLayoutName");
}

int PyDbLayout::getTabOrder() const {
	return impObj()->getTabOrder();
}

void PyDbLayout::setTabOrder(int order) {
	impObj()->setTabOrder(order);
}

bool PyDbLayout::getTabSelected() const {
	return impObj()->getTabSelected();
}

void PyDbLayout::setTabSelected(bool selected) {
	impObj()->setTabSelected(selected ? Adesk::kTrue : Adesk::kFalse);
}

AcString PyDbLayout::toString() const {
	return AcString(std::format("PyDbLayout[tabOrder={}]", getTabOrder()).c_str(), AcString::Utf8);
}

void bind_PyDbLayout(pym& m) {
	py::class_<PyDbLayout, PyDbObject>(m, "PyDbLayout")
		.def(py::init<>())
		.def(py::init<AcDbLayout*>(), py::arg("object"))
		.def("getBlockTableRecordId", &PyDbLayout::getBlockTableRecordId)
		.def("setBlockTableRecordId", &PyDbLayout::setBlockTableRecordId, py::arg("btrId"))
		.def("getLayoutName", &PyDbLayout::getLayoutName)
		.def("setLayoutName", &PyDbLayout::setLayoutName, py::arg("name"))
		.def("getTabOrder", &PyDbLayout::getTabOrder)
		.def("setTabOrder", &PyDbLayout::setTabOrder, py::arg("order"))
		.def("getTabSelected", &PyDbLayout::getTabSelected)
		.def("setTabSelected", &PyDbLayout::setTabSelected, py::arg("selected"))
		.def("getViewportArray", &PyDbLayout::getViewportArray)
		.def("getLimits", &PyDbLayout::getLimits)
		.def("getExtents", &PyDbLayout::getExtents)
		.def("annoAllVisible", &PyDbLayout::annoAllVisible)
		.def("setAnnoAllVisible", &PyDbLayout::setAnnoAllVisible, py::arg("visible"))
		PyWrapper_BIND(DbLayout)
		.def("__repr__", &PyDbLayout::toString)
		;
}
