#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbBlockReference.h"
#include "PyDbAttribute.h"
#include "PyDbDynBlockReference.h"
#include "PyDbDynBlockReferenceProperty.h"
#include "PyDbOpenCache.h"

PyWrapper_Cpp(DbBlockReference)
AcString PyDbBlockReference::toString() const { return AcString(std::format("AcDbBlockReference[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbBlockReference::position() const { if (!impObj()) return AcGePoint3d(); return impObj()->position(); }

bool  PyDbBlockReference::setPosition(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setPosition(p), L"AcDbBlockReference::setPosition"); }

double PyDbBlockReference::rotation() const { if (!impObj()) return 0.0; return impObj()->rotation(); }

bool  PyDbBlockReference::setRotation(double r) { if (!impObj()) return false; return PyWarnIfError(impObj()->setRotation(r), L"AcDbBlockReference::setRotation"); }

uint64_t PyDbBlockReference::blockTableRecordId() const { if (!impObj()) return 0ULL; return static_cast<uint64_t>(impObj()->blockTableRecord().asOldId()); }

bool  PyDbBlockReference::setBlockTableRecord(uint64_t blockRecordId) {
	if (!impObj()) return false;
	// 防御：id 为 0 时立刻抛可读错误，避免后续入库触发 ARX 致命断言 eWrongDatabase。
	if (blockRecordId == 0)
		throw std::runtime_error("setBlockTableRecord: blockRecordId 无效(0)，"
			"请先 bt.getIdAt(blockName) 取有效块定义 id（新建块定义需在事务 commit 之后取）");
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(blockRecordId));
	return PyWarnIfError(impObj()->setBlockTableRecord(id), L"AcDbBlockReference::setBlockTableRecord");
}

AcGeScale3d PyDbBlockReference::scaleFactors() const { return impObj()->scaleFactors(); }

bool  PyDbBlockReference::setScaleFactors(const AcGeScale3d& s) { if (!impObj()) return false; return PyWarnIfError(impObj()->setScaleFactors(s), L"AcDbBlockReference::setScaleFactors"); }

AcGeScale3d PyDbBlockReference::nonAnnotationScaleFactors() const { return impObj()->nonAnnotationScaleFactors(); }

AcGeMatrix3d PyDbBlockReference::blockTransform() const { return impObj()->blockTransform(); }

AcGeMatrix3d PyDbBlockReference::nonAnnotationBlockTransform() const { return impObj()->nonAnnotationBlockTransform(); }

AcGeVector3d PyDbBlockReference::normal() const { if (!impObj()) return AcGeVector3d(); return impObj()->normal(); }

bool  PyDbBlockReference::setNormal(const AcGeVector3d& n) { if (!impObj()) return false; return PyWarnIfError(impObj()->setNormal(n), L"AcDbBlockReference::setNormal"); }

bool PyDbBlockReference::treatAsAcDbBlockRefForExplode() const { if (!impObj()) return false; return impObj()->treatAsAcDbBlockRefForExplode() != Adesk::kFalse; }

bool  PyDbBlockReference::explodeToOwnerSpace() const { if (!impObj()) return false; return PyWarnIfError(impObj()->explodeToOwnerSpace(), L"AcDbBlockReference::explodeToOwnerSpace"); }

std::tuple<AcGePoint3d, AcGePoint3d> PyDbBlockReference::geomExtentsBestFit(const AcGeMatrix3d& parentXform) const {
	if (!impObj()) return {};
	AcDbExtents ext;
	if (!PyWarnIfError(impObj()->geomExtentsBestFit(ext, parentXform), L"AcDbBlockReference::geomExtentsBestFit"))
		return std::make_tuple(AcGePoint3d(), AcGePoint3d());
	return std::make_tuple(ext.minPoint(), ext.maxPoint());
}

bool  PyDbBlockReference::setBlockTransform(const AcGeMatrix3d& xform) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setBlockTransform(xform), L"AcDbBlockReference::setBlockTransform");
}

std::tuple<AcGePlane, int> PyDbBlockReference::getPlane() const {
	if (!impObj()) return {};
	AcGePlane plane; AcDb::Planarity type;
	if (!PyWarnIfError(impObj()->getPlane(plane, type), L"AcDbBlockReference::getPlane"))
		return std::make_tuple(AcGePlane(), 0);
	return std::make_tuple(plane, static_cast<int>(type));
}

bool PyDbBlockReference::isPlanar() const { if (!impObj()) return false; return impObj()->isPlanar() != Adesk::kFalse; }

PyDbAttribute PyDbBlockReference::openAttribute(uint64_t attId, AcDb::OpenMode mode, bool openErasedOne) const {
	if (!impObj()) return {};
	AcDbAttribute* pAttr = nullptr;
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(attId));
	if (!PyWarnIfError(impObj()->openAttribute(pAttr, id, mode, openErasedOne), L"AcDbBlockReference::openAttribute"))
		return PyDbAttribute(nullptr);
	return PyDbAttribute(pAttr);
}

int PyDbBlockReference::attributeCount() const {
	if (!impObj()) return 0;
	AcDbObjectIterator* it = impObj() ? impObj()->attributeIterator() : nullptr;
	if (!it) return -1;
	int n = 0;
	for (; !it->done(); it->step()) ++n;
	delete it;
	return n;
}

py::list PyAttribDataFromObject(AcDbObject* pObj) {
	py::list out;
	AcDbBlockReference* br = pObj ? AcDbBlockReference::cast(pObj) : nullptr;
	if (!br) return out;
	AcDbObjectIterator* it = br->attributeIterator();
	if (!it) return out;
	for (; !it->done(); it->step()) {
		AcDbObjectId aid = it->objectId();
		AcDbAttribute* pa = nullptr;
		if (acdbOpenObject(pa, aid, AcDb::kForRead) != Acad::eOk || !pa)
			continue;
		AcString tag, tv, ly;
		pa->tag(tag);
		pa->textString(tv);
		pa->layer(ly);
		AcGePoint3d p = pa->position();
		AcGePoint3d ap = pa->alignmentPoint();
		py::dict d;
		d["tag"] = tag;
		d["txt"] = tv;
		d["x"] = p.x;
		d["y"] = p.y;
		d["ax"] = ap.x;
		d["ay"] = ap.y;
		d["h"] = pa->height();
		d["rot"] = pa->rotation();
		d["ha"] = static_cast<int>(pa->horizontalMode());
		d["va"] = static_cast<int>(pa->verticalMode());
		d["inv"] = pa->isInvisible() ? true : false;
		d["ly"] = ly;
		d["ci"] = static_cast<int>(pa->colorIndex());
		out.append(d);
		pa->close();
	}
	delete it;
	return out;
}

py::list PyDbBlockReference::attribData() const {
	return PyAttribDataFromObject(impObj());
}

namespace {
// 收集块参照上全部 ATTRIB 的 ObjectId（每次用完即删迭代器，不自持）
std::vector<AcDbObjectId> brAttribIds(AcDbBlockReference* br) {
	std::vector<AcDbObjectId> ids;
	if (!br) return ids;
	AcDbObjectIterator* it = br->attributeIterator();
	if (!it) return ids;
	for (; !it->done(); it->step()) {
		AcDbObjectId aid = it->objectId();
		if (!aid.isNull()) ids.push_back(aid);
	}
	delete it;
	return ids;
}

// 按 id 打开属性，走打开缓存（同对象只允许一个包装，读→写自动 upgrade）
AcDbAttribute* brOpenAttrib(AcDbObjectId id, int mode) {
	AcDb::OpenMode m = static_cast<AcDb::OpenMode>(mode);
	Acad::ErrorStatus es = Acad::eOk;
	AcDbObject* pObj = PyDbOpenCache::Open(id, m, es);
	if ((es != Acad::eOk || !pObj) && (es == Acad::eWasOpenForRead || es == Acad::eWasOpenForWrite)) {
		es = Acad::eOk;
		pObj = PyDbOpenCache::Get(id, m, es);
	}
	if (es != Acad::eOk || !pObj) return nullptr;
	AcDbAttribute* pAtt = AcDbAttribute::cast(pObj);
	if (!pAtt) return nullptr;
	PyDbOpenCache::Track(pAtt, m);
	return pAtt;
}

// 只读取一次 tag 后立即关闭，不留打开痕
bool brPeekTag(AcDbObjectId id, AcString& tag) {
	AcDbAttribute* pAtt = nullptr;
	if (acdbOpenObject(pAtt, id, AcDb::kForRead) != Acad::eOk || !pAtt) return false;
	pAtt->tag(tag);
	pAtt->close();
	return true;
}

// 一次取齐块参照的动态属性（非动态块/失败返回 false，属性arr 为空）
bool brDynProps(AcDbBlockReference* br, AcDbDynBlockReferencePropertyArray& arr) {
	if (!br) return false;
	AcDbObjectId id = br->objectId();
	if (id.isNull()) return false;
	try {
		AcDbDynBlockReference dbr(id);
		if (!dbr.isDynamicBlock()) return false;
		dbr.getBlockProperties(arr);
		return true;
	}
	catch (const std::exception& e) {
		acutPrintf(L"\n[BlockReference] dynamic property: %hs\n", e.what());
		return false;
	}
	catch (...) {
		acutPrintf(L"\n[BlockReference] dynamic property: unknown SDK exception\n");
		return false;
	}
}
} // namespace

py::list PyDbBlockReference::attributes(int mode) const {
	py::list out;
	for (const auto& aid : brAttribIds(impObj())) {
		AcDbAttribute* pAtt = brOpenAttrib(aid, mode);
		if (!pAtt) continue;
		out.append(py::cast(PyDbAttribute(pAtt)));
	}
	return out;
}

PyDbAttribute PyDbBlockReference::attributeAt(int index, int mode) const {
	auto ids = brAttribIds(impObj());
	int total = static_cast<int>(ids.size());
	if (index < 0) index += total;
	if (index < 0 || index >= total) throw py::index_error("BlockReference 属性索引越界");
	AcDbAttribute* pAtt = brOpenAttrib(ids[index], mode);
	return pAtt ? PyDbAttribute(pAtt) : PyDbAttribute(nullptr);
}

PyDbAttribute PyDbBlockReference::attributeByTag(const AcString& tag, int mode) const {
	for (const auto& aid : brAttribIds(impObj())) {
		AcString t;
		if (!brPeekTag(aid, t)) continue;
		// 属性 tag 在 CAD 中不区分大小写
		if (_wcsicmp(t.kwszPtr(), tag.kwszPtr()) != 0) continue;
		AcDbAttribute* pAtt = brOpenAttrib(aid, mode);
		return pAtt ? PyDbAttribute(pAtt) : PyDbAttribute(nullptr);
	}
	return PyDbAttribute(nullptr);
}

bool PyDbBlockReference::hasAttributeTag(const AcString& tag) const {
	for (const auto& aid : brAttribIds(impObj())) {
		AcString t;
		if (!brPeekTag(aid, t)) continue;
		if (_wcsicmp(t.kwszPtr(), tag.kwszPtr()) == 0) return true;
	}
	return false;
}

bool PyDbBlockReference::isDynamicBlock() const {
	AcDbBlockReference* br = impObj();
	if (!br) return false;
	AcDbObjectId id = br->objectId();
	if (id.isNull()) return false;
	try {
		AcDbDynBlockReference dbr(id);
		return dbr.isDynamicBlock();
	}
	catch (const std::exception& e) {
		acutPrintf(L"\n[BlockReference] isDynamicBlock: %hs\n", e.what());
		return false;
	}
	catch (...) {
		acutPrintf(L"\n[BlockReference] isDynamicBlock: unknown SDK exception\n");
		return false;
	}
}

py::list PyDbBlockReference::dynamicProperties() const {
	py::list out;
	AcDbDynBlockReferencePropertyArray arr;
	if (!brDynProps(impObj(), arr)) return out;
	for (int i = 0; i < arr.length(); ++i)
		out.append(py::cast(PyDbDynBlockReferenceProperty(arr[i])));
	return out;
}

py::list PyDbBlockReference::dynamicPropertyNames() const {
	py::list out;
	AcDbDynBlockReferencePropertyArray arr;
	if (!brDynProps(impObj(), arr)) return out;
	for (int i = 0; i < arr.length(); ++i)
		out.append(py::cast(arr[i].propertyName()));
	return out;
}

PyDbDynBlockReferenceProperty PyDbBlockReference::dynamicProperty(const AcString& name) const {
	AcDbDynBlockReferencePropertyArray arr;
	if (!brDynProps(impObj(), arr)) return PyDbDynBlockReferenceProperty();
	for (int i = 0; i < arr.length(); ++i) {
		if (_wcsicmp(arr[i].propertyName().kwszPtr(), name.kwszPtr()) == 0)
			return PyDbDynBlockReferenceProperty(arr[i]);
	}
	return PyDbDynBlockReferenceProperty();
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
			if (!self.impObj()) return out;
			AcDbObjectIterator* it = self.impObj()->attributeIterator();
			if (!it) return out;
			for (; !it->done(); it->step())
				out.append(static_cast<uint64_t>(it->objectId().asOldId()));
			delete it;
			return out;
		}, "返回块参照上所有属性的 id 列表")
		.def("hasAttributes", [](const PyDbBlockReference& d) { return d.attributeCount() > 0; }, "是否含属性（属性数 > 0）")
		.def("attributeCount", &PyDbBlockReference::attributeCount, "属性数量（-1 表示迭代器创建失败）")
		.def("attribData", &PyDbBlockReference::attribData,
		     "实例属性列表：{tag,txt,x,y,ax,ay,h,rot,ha,va,inv,ly,ci}")
		.def("appendAttribute", [](PyDbBlockReference& self, PyDbAttribute& attr) {
			return PyWarnIfError(self.impObj()->appendAttribute(attr.impObj()), L"appendAttribute");
		}, py::arg("attribute"), "向块参照追加一个属性")
		// ---- 属性块：ATTRIB 遍历（原 attributeIterator 折叠进本体）----
		.def("attributes", &PyDbBlockReference::attributes, py::arg("mode") = 1,
			"blkref.attributes(mode)：属性对象列表（mode 0=只读 1=写，返回的是已打开的包装）")
		.def("attributeAt", &PyDbBlockReference::attributeAt,
			py::arg("index"), py::arg("mode") = 1,
			"blkref.attributeAt(i)：按位置取属性（支持负索引，越界抛 IndexError）")
		.def("attributeByTag", &PyDbBlockReference::attributeByTag,
			py::arg("tag"), py::arg("mode") = 1,
			"blkref.attributeByTag(tag)：按 tag 取属性（不区分大小写，不存在返回空对象）")
		.def("hasAttributeTag", &PyDbBlockReference::hasAttributeTag, py::arg("tag"),
			"是否存在指定 tag 的属性（不区分大小写）")
		.def("__len__", [](const PyDbBlockReference& self) {
			int n = self.attributeCount();
			return n < 0 ? 0 : n;
		}, "len(blkref)：属性数量")
		.def("__getitem__", [](const PyDbBlockReference& self, int index) {
			return self.attributeAt(index, 1);
		}, py::arg("index"), "blkref[i]：按位置取属性（写模式）")
		.def("__iter__", [](const PyDbBlockReference& self) {
			return self.attributes(0).attr("__iter__")();
		}, "for att in blkref：遍历全部属性（只读）")
		// ---- 动态块：动态属性遍历（原 DynBlockReference.getBlockProperties 折叠进本体）----
		.def("isDynamicBlock", &PyDbBlockReference::isDynamicBlock, "是否为动态块参照")
		.def("dynamicProperties", &PyDbBlockReference::dynamicProperties,
			"blkref.dynamicProperties()：动态属性对象列表（非动态块返回空 list）")
		.def("dynamicPropertyNames", &PyDbBlockReference::dynamicPropertyNames,
			"blkref.dynamicPropertyNames()：动态属性名列表")
		.def("dynamicProperty", &PyDbBlockReference::dynamicProperty, py::arg("name"),
			"blkref.dynamicProperty(name)：按名取动态属性（不存在返回空对象）")
		PyWrapper_BIND(DbBlockReference)
		.def("__repr__", &PyDbBlockReference::toString)
		;
}
