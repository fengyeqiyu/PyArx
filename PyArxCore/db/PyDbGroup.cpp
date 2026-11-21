#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyUtils.h"
#include "PyDbGroup.h"

PyWrapper_Cpp(DbGroup);

bool  PyDbGroup::addEntity(PyDbEntity& e) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->append(e.impObj()->objectId()), L"Group.append");
}

bool  PyDbGroup::removeEntity(PyDbEntity& e) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->remove(e.impObj()->objectId()), L"Group.remove");
}

bool PyDbGroup::hasEntity(PyDbEntity& e) const {
	if (!impObj()) return false;
	return impObj()->has(e.impObj());
}

AcString PyDbGroup::getName() const {
	if (!impObj()) return AcString();
	AcString s;
	if(!PyWarnIfError(impObj()->getName(s), L"Group.getName")) return AcString();
	return s;
}

bool  PyDbGroup::setName(const AcString& s) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setName(s.kwszPtr()), L"Group.setName");
}

bool PyDbGroup::isAnonymous() const {
	if (!impObj()) return false;
	return impObj()->isAnonymous();
}

bool PyDbGroup::isNotAccessible() const {
	if (!impObj()) return false;
	return impObj()->isNotAccessible();
}

bool  PyDbGroup::clear() {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->clear(), L"Group.clear");
}

bool  PyDbGroup::reverse() {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->reverse(), L"Group.reverse");
}

AcString PyDbGroup::description() const {
	if (!impObj()) return AcString();
	const ACHAR* p = impObj()->description();
	return p ? AcString(p) : AcString();
}

bool  PyDbGroup::setDescription(const AcString& s) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setDescription(s.kwszPtr()), L"Group.setDescription");
}

int PyDbGroup::numEntities() const {
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->numEntities());
}

bool PyDbGroup::isSelectable() const {
	if (!impObj()) return false;
	return impObj()->isSelectable();
}

bool PyDbGroup::setSelectable(bool selectable) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setSelectable(selectable), L"Group.setSelectable");
}

py::list PyDbGroup::allEntityIds() const {
	if (!impObj()) return py::list();
	AcDbObjectIdArray ids;
	impObj()->allEntityIds(ids);
	return py::cast(ids);
}

int PyDbGroup::getIndex(uint64_t objId) const {
	if (!impObj()) return 0;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	Adesk::UInt32 idx = 0;
	if (!PyWarnIfError(impObj()->getIndex(id, idx), L"Group.getIndex")) return -1;
	return static_cast<int>(idx);
}

bool PyDbGroup::removeAt(int index) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->removeAt(static_cast<Adesk::UInt32>(index)), L"Group.removeAt");
}

bool PyDbGroup::replace(uint64_t oldId, uint64_t newId) {
	if (!impObj()) return false;
	AcDbObjectId oOld, oNew;
	oOld.setFromOldId(static_cast<Adesk::IntDbId>(oldId));
	oNew.setFromOldId(static_cast<Adesk::IntDbId>(newId));
	return PyWarnIfError(impObj()->replace(oOld, oNew), L"Group.replace");
}

bool PyDbGroup::prepend(uint64_t objId) {
	if (!impObj()) return false;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	return PyWarnIfError(impObj()->prepend(id), L"Group.prepend");
}

bool PyDbGroup::insertAt(int index, uint64_t objId) {
	if (!impObj()) return false;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(objId));
	return PyWarnIfError(impObj()->insertAt(static_cast<Adesk::UInt32>(index), id), L"Group.insertAt");
}

bool PyDbGroup::setColorIndex(int colorIndex) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setColorIndex(static_cast<Adesk::UInt16>(colorIndex)), L"Group.setColorIndex");
}

bool PyDbGroup::setLayerByName(const AcString& layerName) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setLayer(layerName.kwszPtr()), L"Group.setLayerByName");
}

bool PyDbGroup::setLinetypeByName(const AcString& ltypeName) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setLinetype(ltypeName.kwszPtr()), L"Group.setLinetypeByName");
}

bool PyDbGroup::setLinetypeScale(double scale) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setLinetypeScale(scale), L"Group.setLinetypeScale");
}

bool PyDbGroup::setVisibility(int visibility) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setVisibility(static_cast<AcDb::Visibility>(visibility)), L"Group.setVisibility");
}

bool PyDbGroup::setHighlight(bool highlight) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setHighlight(highlight), L"Group.setHighlight");
}

bool PyDbGroup::setAnonymous() {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setAnonymous(), L"Group.setAnonymous");
}

bool PyDbGroup::setMaterial(const AcString& materialName) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setMaterial(materialName.kwszPtr()), L"Group.setMaterial");
}

bool PyDbGroup::transfer(int fromIndex, int toIndex, int numItems) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->transfer(static_cast<Adesk::UInt32>(fromIndex),
		static_cast<Adesk::UInt32>(toIndex), static_cast<Adesk::UInt32>(numItems)), L"Group.transfer");
}

AcString PyDbGroup::toString() const {
	return AcString(std::format("PyDbGroup[numEntities={}]", numEntities()).c_str(), AcString::Utf8);
}

void bind_PyDbGroup(pym& m) {
	py::class_<PyDbGroup, PyDbObject>(m, "PyDbGroup")
		.def(py::init<>())
		.def(py::init<const AcString&, bool>(), py::arg("desc"), py::arg("selectable") = true)
		.def(py::init<AcDbGroup*>(), py::arg("object"))
		.def("addEntity", &PyDbGroup::addEntity, py::arg("e"))
		.def("removeEntity", &PyDbGroup::removeEntity, py::arg("e"))
		.def("hasEntity", &PyDbGroup::hasEntity, py::arg("e"))
		.def("getName", &PyDbGroup::getName)
		.def("setName", &PyDbGroup::setName, py::arg("s"))
		.def("isAnonymous", &PyDbGroup::isAnonymous)
		.def("isNotAccessible", &PyDbGroup::isNotAccessible)
		.def("clear", &PyDbGroup::clear)
		.def("reverse", &PyDbGroup::reverse)
		.def("description", &PyDbGroup::description)
		.def("setDescription", &PyDbGroup::setDescription, py::arg("s"))
		.def("numEntities", &PyDbGroup::numEntities)
		.def("isSelectable", &PyDbGroup::isSelectable)
		.def("setSelectable", &PyDbGroup::setSelectable, py::arg("selectable"))
		.def("allEntityIds", &PyDbGroup::allEntityIds)
		.def("getIndex", &PyDbGroup::getIndex, py::arg("objId"))
		.def("removeAt", &PyDbGroup::removeAt, py::arg("index"))
		.def("replace", &PyDbGroup::replace, py::arg("oldId"), py::arg("newId"))
		.def("prepend", &PyDbGroup::prepend, py::arg("objId"))
		.def("insertAt", &PyDbGroup::insertAt, py::arg("index"), py::arg("objId"))
		.def("setColorIndex", &PyDbGroup::setColorIndex, py::arg("colorIndex"))
		.def("setLayerByName", &PyDbGroup::setLayerByName, py::arg("layerName"))
		.def("setLinetypeByName", &PyDbGroup::setLinetypeByName, py::arg("ltypeName"))
		.def("setLinetypeScale", &PyDbGroup::setLinetypeScale, py::arg("scale"))
		.def("setVisibility", &PyDbGroup::setVisibility, py::arg("visibility"))
		.def("setHighlight", &PyDbGroup::setHighlight, py::arg("highlight"))
		.def("setAnonymous", &PyDbGroup::setAnonymous)
		.def("setMaterial", &PyDbGroup::setMaterial, py::arg("materialName"))
		.def("transfer", &PyDbGroup::transfer, py::arg("fromIndex"), py::arg("toIndex"), py::arg("numItems"))
		PyWrapper_BIND(DbGroup)
		.def("__repr__", &PyDbGroup::toString)
		;
}
