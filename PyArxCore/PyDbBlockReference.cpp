#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbBlockReference.h"
#include "PyDbAttribute.h"

PyWrapper_Cpp(DbBlockReference)
AcString PyDbBlockReference::toString() const { return AcString(std::format("AcDbBlockReference[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbBlockReference::position() const { return impObj()->position(); }

bool  PyDbBlockReference::setPosition(const AcGePoint3d& p) { return PyWarnIfError(impObj()->setPosition(p), L"AcDbBlockReference::setPosition"); }

double PyDbBlockReference::rotation() const { return impObj()->rotation(); }

bool  PyDbBlockReference::setRotation(double r) { return PyWarnIfError(impObj()->setRotation(r), L"AcDbBlockReference::setRotation"); }

uint64_t PyDbBlockReference::blockTableRecordId() const { return static_cast<uint64_t>(impObj()->blockTableRecord().asOldId()); }

bool  PyDbBlockReference::setBlockTableRecord(uint64_t blockRecordId) {
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(blockRecordId));
	return PyWarnIfError(impObj()->setBlockTableRecord(id), L"AcDbBlockReference::setBlockTableRecord");
}

AcGeScale3d PyDbBlockReference::scaleFactors() const { return impObj()->scaleFactors(); }

bool  PyDbBlockReference::setScaleFactors(const AcGeScale3d& s) { return PyWarnIfError(impObj()->setScaleFactors(s), L"AcDbBlockReference::setScaleFactors"); }

AcGeScale3d PyDbBlockReference::nonAnnotationScaleFactors() const { return impObj()->nonAnnotationScaleFactors(); }

AcGeMatrix3d PyDbBlockReference::blockTransform() const { return impObj()->blockTransform(); }

AcGeMatrix3d PyDbBlockReference::nonAnnotationBlockTransform() const { return impObj()->nonAnnotationBlockTransform(); }

AcGeVector3d PyDbBlockReference::normal() const { return impObj()->normal(); }

bool  PyDbBlockReference::setNormal(const AcGeVector3d& n) { return PyWarnIfError(impObj()->setNormal(n), L"AcDbBlockReference::setNormal"); }

bool PyDbBlockReference::treatAsAcDbBlockRefForExplode() const { return impObj()->treatAsAcDbBlockRefForExplode() != Adesk::kFalse; }

bool  PyDbBlockReference::explodeToOwnerSpace() const { return PyWarnIfError(impObj()->explodeToOwnerSpace(), L"AcDbBlockReference::explodeToOwnerSpace"); }

std::tuple<AcGePoint3d, AcGePoint3d> PyDbBlockReference::geomExtentsBestFit(const AcGeMatrix3d& parentXform) const {
	AcDbExtents ext;
	if (!PyWarnIfError(impObj()->geomExtentsBestFit(ext, parentXform), L"AcDbBlockReference::geomExtentsBestFit"))
		return std::make_tuple(AcGePoint3d(), AcGePoint3d());
	return std::make_tuple(ext.minPoint(), ext.maxPoint());
}

bool  PyDbBlockReference::setBlockTransform(const AcGeMatrix3d& xform) {
	return PyWarnIfError(impObj()->setBlockTransform(xform), L"AcDbBlockReference::setBlockTransform");
}

std::tuple<AcGePlane, int> PyDbBlockReference::getPlane() const {
	AcGePlane plane; AcDb::Planarity type;
	if (!PyWarnIfError(impObj()->getPlane(plane, type), L"AcDbBlockReference::getPlane"))
		return std::make_tuple(AcGePlane(), 0);
	return std::make_tuple(plane, static_cast<int>(type));
}

bool PyDbBlockReference::isPlanar() const { return impObj()->isPlanar() != Adesk::kFalse; }

PyDbAttribute PyDbBlockReference::openAttribute(uint64_t attId, AcDb::OpenMode mode, bool openErasedOne) const {
	AcDbAttribute* pAttr = nullptr;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(attId));
	if (!PyWarnIfError(impObj()->openAttribute(pAttr, id, mode, openErasedOne), L"AcDbBlockReference::openAttribute"))
		return PyDbAttribute(nullptr);
	return PyDbAttribute(pAttr);
}

void bind_PyDbBlockReference(pym& m) {
	py::class_<PyDbBlockReference, PyDbEntity>(m, "PyDbBlockReference")
		.def(py::init<>())
		.def(py::init<AcDbBlockReference*>(), py::arg("object"))
		.def("position", &PyDbBlockReference::position)
		.def("setPosition", &PyDbBlockReference::setPosition, py::arg("p"))
		.def("rotation", &PyDbBlockReference::rotation)
		.def("setRotation", &PyDbBlockReference::setRotation, py::arg("r"))
		.def("blockTableRecordId", &PyDbBlockReference::blockTableRecordId)
		.def("setBlockTableRecord", &PyDbBlockReference::setBlockTableRecord, py::arg("blockRecordId"))
		.def("scaleFactors", &PyDbBlockReference::scaleFactors)
		.def("setScaleFactors", &PyDbBlockReference::setScaleFactors, py::arg("s"))
		.def("nonAnnotationScaleFactors", &PyDbBlockReference::nonAnnotationScaleFactors)
		.def("blockTransform", &PyDbBlockReference::blockTransform)
		.def("nonAnnotationBlockTransform", &PyDbBlockReference::nonAnnotationBlockTransform)
		.def("normal", &PyDbBlockReference::normal)
		.def("setNormal", &PyDbBlockReference::setNormal, py::arg("n"))
		.def("treatAsAcDbBlockRefForExplode", &PyDbBlockReference::treatAsAcDbBlockRefForExplode)
		.def("explodeToOwnerSpace", &PyDbBlockReference::explodeToOwnerSpace)
		.def("geomExtentsBestFit", &PyDbBlockReference::geomExtentsBestFit, py::arg("parentXform") = AcGeMatrix3d::kIdentity,
			"返回块参照最佳拟合包围盒 (minPoint, maxPoint)")
		.def("setBlockTransform", &PyDbBlockReference::setBlockTransform, py::arg("xform"), "设置块参照变换矩阵")
		.def("getPlane", &PyDbBlockReference::getPlane, "返回 (plane, planarity 枚举 int)")
		.def("isPlanar", &PyDbBlockReference::isPlanar)
		.def("openAttribute", &PyDbBlockReference::openAttribute,
			py::arg("attId"), py::arg("mode"), py::arg("openErasedOne") = false,
			"按 ObjectId 打开块参照上的属性（失败返回空 wrapper）")
		// ---- 属性 ----
		.def("attributeIds", [](const PyDbBlockReference& self) -> py::list {
			py::list out;
			AcDbObjectIterator* it = self.impObj()->attributeIterator();
			if (!it) return out;
			for (; !it->done(); it->step())
				out.append(static_cast<uint64_t>(it->objectId().asOldId()));
			delete it;
			return out;
		}, "返回块参照上所有属性的 id 列表")
		.def("appendAttribute", [](PyDbBlockReference& self, PyDbAttribute& attr) {
			return PyWarnIfError(self.impObj()->appendAttribute(attr.impObj()), L"appendAttribute");
		}, py::arg("attribute"), "向块参照追加一个属性")
		PyWrapper_BIND(DbBlockReference)
		.def("__repr__", &PyDbBlockReference::toString)
		;
}
