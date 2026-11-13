#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbGeoData.h"

PyWrapper_Cpp(DbGeoData);

uint64_t PyDbGeoData::blockTableRecordId() const {
	AcDbObjectId id = impObj()->blockTableRecordId();
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
int PyDbGeoData::coordinateType() const { return (int)impObj()->coordinateType(); }
AcGePoint3d PyDbGeoData::designPoint() const { return impObj()->designPoint(); }
bool PyDbGeoData::eraseFromDb() {
	return PyWarnIfError(impObj()->eraseFromDb(), L"AcDbGeoData::eraseFromDb");
}
std::tuple<bool, uint64_t> PyDbGeoData::postToDb() {
	AcDbObjectId id;
	Acad::ErrorStatus es = impObj()->postToDb(id);
	if (!PyWarnIfError(es, L"AcDbGeoData::postToDb")) return std::make_tuple(false, 0ULL);
	return std::make_tuple(true, id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()));
}
bool PyDbGeoData::setBlockTableRecordId(uint64_t blockId) {
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(blockId));
	return PyWarnIfError(impObj()->setBlockTableRecordId(id), L"AcDbGeoData::setBlockTableRecordId");
}
bool PyDbGeoData::setCoordinateType(int t) {
	return PyWarnIfError(impObj()->setCoordinateType(static_cast<AcDbGeoData::TypeOfCoordinates>(t)), L"AcDbGeoData::setCoordinateType");
}
bool PyDbGeoData::setDesignPoint(const AcGePoint3d& p) {
	return PyWarnIfError(impObj()->setDesignPoint(p), L"AcDbGeoData::setDesignPoint");
}

void bind_PyDbGeoData(pym& m) {
	py::class_<PyDbGeoData, PyDbObject>(m, "PyDbGeoData")
		.def(py::init<AcDbGeoData*>(), py::arg("p"))
		.def(py::init([]() { return std::make_unique<PyDbGeoData>(new AcDbGeoData()).release(); }), "无参构造（new 默认 AcDbGeoData）")
		PyWrapper_BIND(DbGeoData)
		.def("blockTableRecordId", &PyDbGeoData::blockTableRecordId)
		.def("coordinateType", &PyDbGeoData::coordinateType)
		.def("designPoint", &PyDbGeoData::designPoint)
		.def("eraseFromDb", &PyDbGeoData::eraseFromDb)
		.def("postToDb", &PyDbGeoData::postToDb, "返回 (ok, outObjectId)")
		.def("setBlockTableRecordId", &PyDbGeoData::setBlockTableRecordId, py::arg("blockId"))
		.def("setCoordinateType", &PyDbGeoData::setCoordinateType, py::arg("t"))
		.def("setDesignPoint", &PyDbGeoData::setDesignPoint, py::arg("p"))
		;
}
