#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyUtils.h"
#include "PyDbLayout.h"

PyWrapper_Cpp(DbLayout);

uint64_t PyDbLayout::getBlockTableRecordId() const {
	if (!impObj()) return 0;
	return static_cast<uint64_t>(impObj()->getBlockTableRecordId().asOldId());
}

bool PyDbLayout::setBlockTableRecordId(uint64_t btrId) {
	if (!impObj()) return false;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(btrId));
	return PyWarnIfError(impObj()->setBlockTableRecordId(id), L"Layout.setBlockTableRecordId");
}

py::list PyDbLayout::getViewportArray() const {
	if (!impObj()) return py::list();
	return py::cast(impObj()->getViewportArray());
}

py::tuple PyDbLayout::getLimits() const {
	if (!impObj()) return py::tuple();
	AcGePoint2d limMin, limMax;
	impObj()->getLimits(limMin, limMax);
	return py::make_tuple(
		py::make_tuple(limMin.x, limMin.y),
		py::make_tuple(limMax.x, limMax.y));
}

py::tuple PyDbLayout::getExtents() const {
	if (!impObj()) return py::tuple();
	AcGePoint3d extMin, extMax;
	impObj()->getExtents(extMin, extMax);
	return py::make_tuple(
		py::make_tuple(extMin.x, extMin.y, extMin.z),
		py::make_tuple(extMax.x, extMax.y, extMax.z));
}

bool PyDbLayout::annoAllVisible() const {
	if (!impObj()) return false;
	return impObj()->annoAllVisible();
}

bool PyDbLayout::setAnnoAllVisible(bool visible) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setAnnoAllVisible(visible), L"Layout.setAnnoAllVisible");
}

AcString PyDbLayout::getLayoutName() const {
	if (!impObj()) return AcString();
	AcString s;
	if(!PyWarnIfError(impObj()->getName(s), L"Layout.getName")) return AcString();
	return s;
}

bool  PyDbLayout::setLayoutName(const AcString& name) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setLayoutName(name.kwszPtr()), L"Layout.setLayoutName");
}

int PyDbLayout::getTabOrder() const {
	if (!impObj()) return 0;
	return impObj()->getTabOrder();
}

void PyDbLayout::setTabOrder(int order) {
	if (!impObj()) return;
	impObj()->setTabOrder(order);
}

bool PyDbLayout::getTabSelected() const {
	if (!impObj()) return false;
	return impObj()->getTabSelected();
}

void PyDbLayout::setTabSelected(bool selected) {
	if (!impObj()) return;
	impObj()->setTabSelected(selected ? Adesk::kTrue : Adesk::kFalse);
}

AcString PyDbLayout::toString() const {
	if (!impObj()) return AcString();
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
