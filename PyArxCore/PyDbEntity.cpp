#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbEntity.h"

PyWrapper_Cpp(DbEntity)

unsigned short PyDbEntity::colorIndex() const { if (!impObj()) return 0; return impObj()->colorIndex(); }
bool  PyDbEntity::setColorIndex(unsigned short colorIndex, bool doSubents) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setColorIndex(static_cast<Adesk::UInt16>(colorIndex), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setColorIndex");
}

AcString PyDbEntity::layer() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->layer(s), L"AcDbEntity::layer")) return AcString(); return s; }
uint64_t PyDbEntity::layerId() const { if (!impObj()) return 0ULL; return static_cast<uint64_t>(impObj()->layerId().asOldId()); }
bool  PyDbEntity::setLayer(const AcString& name, bool doSubents) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setLayer(name.kwszPtr(), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setLayer");
}

bool  PyDbEntity::setLayerId(uint64_t layerId, bool doSubents) {
	if (!impObj()) return false;
	// 与 setBlockTableRecord / openAttribute 一致：Python 侧用 oldId(uint64) 表示
	// 对象 ID，这里还原成 AcDbObjectId 走 id 版 setLayer，绕开字符串转换。
	if (layerId == 0) {
		throw std::runtime_error("setLayerId: layerId 无效(0)，"
			"请先 lt.getAt(name).objectId() 取图层 id");
	}
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::IntDbId>(layerId));
	return PyWarnIfError(impObj()->setLayer(id, doSubents ? Adesk::kTrue : Adesk::kFalse),
		L"AcDbEntity::setLayer(id)");
}

AcString PyDbEntity::linetype() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->linetype(s), L"AcDbEntity::linetype")) return AcString(); return s; }
uint64_t PyDbEntity::linetypeId() const { if (!impObj()) return 0ULL; return static_cast<uint64_t>(impObj()->linetypeId().asOldId()); }
bool  PyDbEntity::setLinetype(const AcString& name, bool doSubents) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setLinetype(name.kwszPtr(), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setLinetype");
}

int PyDbEntity::lineWeight() const { if (!impObj()) return 0; return static_cast<int>(impObj()->lineWeight()); }
bool  PyDbEntity::setLineWeight(int newVal, bool doSubents) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setLineWeight(static_cast<AcDb::LineWeight>(newVal), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setLineWeight");
}

int PyDbEntity::visibility() const { if (!impObj()) return 0; return static_cast<int>(impObj()->visibility()); }
bool  PyDbEntity::setVisibility(int newVal, bool doSubents) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setVisibility(static_cast<AcDb::Visibility>(newVal), doSubents), L"AcDbEntity::setVisibility");
}

double PyDbEntity::linetypeScale() const { if (!impObj()) return 0.0; return impObj()->linetypeScale(); }
bool  PyDbEntity::setLinetypeScale(double newval, bool doSubents) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setLinetypeScale(newval, doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setLinetypeScale");
}

py::object PyDbEntity::getGeomExtents() const {
	if (!impObj()) return py::object();
	AcDbExtents ext;
	if(!PyWarnIfError(impObj()->getGeomExtents(ext), L"AcDbEntity::getGeomExtents")) return py::none();
	return py::make_tuple(ext.minPoint(), ext.maxPoint());
}

bool  PyDbEntity::transformBy(const AcGeMatrix3d& xform) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->transformBy(xform), L"AcDbEntity::transformBy");
}

py::list PyDbEntity::intersectWith(const PyDbEntity& other, int intType) const {
	if (!impObj()) return py::list();
	py::list out;
	AcGePoint3dArray pts;
	if(!PyWarnIfError(impObj()->intersectWith(other.impObj(), static_cast<AcDb::Intersect>(intType), pts), L"AcDbEntity::intersectWith")) return out;
	for(int i = 0; i < pts.length(); ++i) out.append(pts[i]);
	return out;
}

// ---- 颜色 / 透明度（AcCm* 未单独绑定，提取为基本类型）----
PyColor PyDbEntity::color() const {
	if (!impObj()) return {};
	return PyColor(impObj()->color());
}
bool PyDbEntity::setColorByRgb(int r, int g, int b, bool doSubents) {
	if (!impObj()) return false;
	AcCmColor c;
	auto clamp = [](int v) { return static_cast<Adesk::UInt8>(v < 0 ? 0 : (v > 255 ? 255 : v)); };
	c.setRGB(clamp(r), clamp(g), clamp(b));
	return PyWarnIfError(impObj()->setColor(c, doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setColor");
}
std::tuple<int,int,int> PyDbEntity::entityColor() const {
	if (!impObj()) return {};
	AcCmEntityColor c = impObj()->entityColor();
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
std::tuple<int,int,int> PyDbEntity::colorRGB() const {
	if (!impObj()) return {};
	AcCmColor c = impObj()->color();
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
// SDK 只提供 ByLayer 解析(accmResolveEffectiveColorToRGB)，ByBlock 继承、块内 "0" 图层、
// 257 不绘制、图层关闭/冻结均需在此自行补齐；前景色(ACI 7) 按黑底反色，与 accm 一致。
static bool pyLayerOffOrFrozen(AcDbObjectId layerId, bool& off, bool& frozen) {
	AcDbLayerTableRecord* pLay = nullptr;
	if (acdbOpenObject(pLay, layerId, AcDb::kForRead) != Acad::eOk || !pLay) return false;
	off = (pLay->isOff() == Adesk::kTrue);
	frozen = (pLay->isFrozen() == Adesk::kTrue);
	pLay->close();
	return true;
}
std::tuple<int,int,int> PyDbEntity::displayColor() const {
	if (!impObj()) return {};
	AcCmEntityColor ec = impObj()->entityColor();
	if (ec.isNone()) return std::make_tuple(0, 0, 0);
	if (ec.isByColor()) return std::make_tuple((int)ec.red(), (int)ec.green(), (int)ec.blue());

	AcCmEntityColor eff = ec;
	AcDbObjectId layerId = impObj()->layerId();
	bool insideBlock = false;
	AcDbObjectId blockLayerId = AcDbObjectId::kNull;

	if (ec.isByBlock()) {
		AcDbObjectId ownerId = impObj()->ownerId();
		for (int depth = 0; depth < 16; ++depth) {   // 嵌套块向上继承，限深防环
			AcDbObject* pOwner = nullptr;
			if (acdbOpenObject(pOwner, ownerId, AcDb::kForRead) != Acad::eOk || !pOwner) break;
			AcDbBlockReference* pBr = AcDbBlockReference::cast(pOwner);
			if (!pBr) { pOwner->close(); break; }
			AcCmEntityColor brc = pBr->entityColor();
			ownerId = pBr->ownerId();
			insideBlock = true;
			blockLayerId = pBr->layerId();
			pOwner->close();
			if (!brc.isByBlock()) { eff = brc; layerId = blockLayerId; break; }
		}
	}
	if (eff.isByBlock()) eff = AcCmEntityColor::ByLayer();   // 顶层无块上下文 → 随图层

	if (insideBlock && blockLayerId != AcDbObjectId::kNull) {
		const wchar_t* name = impObj()->layer();
		if (name != nullptr && 0 == wcscmp(name, L"0")) layerId = blockLayerId;  // 块内 "0" 层随父块图层
	}
	AcCmEntityColor out = accmResolveEffectiveColorToRGB(eff, layerId);
	return std::make_tuple((int)out.red(), (int)out.green(), (int)out.blue());
}
bool PyDbEntity::isColorVisible() const {
	if (!impObj()) return false;
	if (impObj()->entityColor().isNone()) return false;
	if (impObj()->visibility() == AcDb::kInvisible) return false;
	bool off = false, frozen = false;
	if (pyLayerOffOrFrozen(impObj()->layerId(), off, frozen) && (off || frozen)) return false;
	return true;
}
int  PyDbEntity::transparency() const { if (!impObj()) return 0; return (int)impObj()->transparency().alpha(); }
bool PyDbEntity::setTransparency(int alpha, bool doSubents) {
	if (!impObj()) return false;
	AcCmTransparency t(static_cast<Adesk::UInt8>(alpha));
	return PyWarnIfError(impObj()->setTransparency(t, doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setTransparency");
}

// ---- 包围盒 ----
std::tuple<AcGePoint3d,AcGePoint3d> PyDbEntity::bounds() const {
	if (!impObj()) return {};
	AcDbExtents ext;
	if(!impObj()->bounds(ext)) return std::make_tuple(AcGePoint3d(), AcGePoint3d());
	return std::make_tuple(ext.minPoint(), ext.maxPoint());
}

// ---- 平面性 ----
std::tuple<AcGePlane,int> PyDbEntity::getPlane() const {
	if (!impObj()) return {};
	AcGePlane pl; AcDb::Planarity pa;
	if(!PyWarnIfError(impObj()->getPlane(pl, pa), L"AcDbEntity::getPlane")) return std::make_tuple(AcGePlane(), (int)-1);
	return std::make_tuple(pl, (int)pa);
}
bool PyDbEntity::isPlanar() const { if (!impObj()) return false; return impObj()->isPlanar() != Adesk::kFalse; }
bool PyDbEntity::isContentSnappable() const { if (!impObj()) return false; return impObj()->isContentSnappable(); }

// ---- 材质 ----
AcString PyDbEntity::material() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->material(s), L"AcDbEntity::material")) return AcString(); return s; }
uint64_t PyDbEntity::materialId() const { if (!impObj()) return 0ULL; return static_cast<uint64_t>(impObj()->materialId().asOldId()); }
bool  PyDbEntity::setMaterial(const AcString& name, bool doSubents) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setMaterial(name.kwszPtr(), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setMaterial");
}

// ---- 数据库默认值 / 属性复制 ----
void PyDbEntity::setDatabaseDefaults() { if (!impObj()) return; impObj()->setDatabaseDefaults(); }
bool PyDbEntity::setPropertiesFrom(const PyDbEntity& src, bool doSubents) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setPropertiesFrom(src.impObj(), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setPropertiesFrom");
}

// ---- 阴影 ----
bool PyDbEntity::castShadows() const { if (!impObj()) return false; return impObj()->castShadows(); }
bool PyDbEntity::setCastShadows(bool newVal) { if (!impObj()) return false; impObj()->setCastShadows(newVal); return true; }
bool PyDbEntity::receiveShadows() const { if (!impObj()) return false; return impObj()->receiveShadows(); }
bool PyDbEntity::setReceiveShadows(bool newVal) { if (!impObj()) return false; impObj()->setReceiveShadows(newVal); return true; }

// ---- 其它显示 / 图形状态 ----
int  PyDbEntity::collisionType() const { if (!impObj()) return 0; return static_cast<int>(impObj()->collisionType()); }
void PyDbEntity::recordGraphicsModified(bool setModified) { if (!impObj()) return; impObj()->recordGraphicsModified(setModified ? Adesk::kTrue : Adesk::kFalse); }
uint64_t PyDbEntity::visualStyleId() const { if (!impObj()) return 0ULL; return static_cast<uint64_t>(impObj()->visualStyleId().asOldId()); }
bool PyDbEntity::setVisualStyle(uint64_t visualStyleId, bool doSubents) {
	if (!impObj()) return false;
	AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::IntDbId>(visualStyleId));
	return PyWarnIfError(impObj()->setVisualStyle(oid, AcDbEntity::kFullVisualStyle, doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setVisualStyle");
}
uint64_t PyDbEntity::blockId() const { if (!impObj()) return 0ULL; return static_cast<uint64_t>(impObj()->blockId().asOldId()); }
AcGeMatrix3d PyDbEntity::getEcs() const { if (!impObj()) return AcGeMatrix3d(); AcGeMatrix3d m; impObj()->getEcs(m); return m; }

// ---- 打印样式 ----
AcString PyDbEntity::plotStyleName() const { if (!impObj()) return AcString(); AcString s; impObj()->plotStyleName(s); return s; }
bool PyDbEntity::setPlotStyleName(const AcString& name) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setPlotStyleName(name.kwszPtr()), L"AcDbEntity::setPlotStyleName");
}

// ---- 其它 ----
void PyDbEntity::list() const { if (!impObj()) return; impObj()->list(); }
void PyDbEntity::draw() const { if (!impObj()) return; impObj()->draw(); }

py::tuple PyDbEntity::getGripPoints() const {
	if (!impObj()) return py::tuple();
	AcGePoint3dArray gripPoints;
	AcDbIntArray osnapModes, geomIds;
	if (!PyWarnIfError(impObj()->getGripPoints(gripPoints, osnapModes, geomIds), L"AcDbEntity::getGripPoints")) return py::tuple();
	py::list pyGrip, pyOsnap, pyGeom;
	for (int i = 0; i < gripPoints.length(); i++) pyGrip.append(gripPoints[i]);
	for (int i = 0; i < osnapModes.length(); i++) pyOsnap.append(osnapModes[i]);
	for (int i = 0; i < geomIds.length(); i++) pyGeom.append(geomIds[i]);
	return py::make_tuple(pyGrip, pyOsnap, pyGeom);
}

void PyDbEntity::moveGripPointsAt(const std::vector<int>& indices, const AcGeVector3d& offset) {
	if (!impObj()) return;
	AcDbIntArray idx;
	for (int i : indices) idx.append(i);
	PyWarnIfError(impObj()->moveGripPointsAt(idx, offset), L"AcDbEntity::moveGripPointsAt");
}

// --- 高亮 ---
bool PyDbEntity::pushHighlight(int style) {
	if (!impObj()) return false;
	AcDbFullSubentPath path;
	Acad::ErrorStatus es = impObj()->highlight(path, Adesk::kTrue);
	return PyWarnIfError(es, L"PyDbEntity::pushHighlight");
}

bool PyDbEntity::popHighlight() {
	if (!impObj()) return false;
	AcDbFullSubentPath path;
	Acad::ErrorStatus es = impObj()->highlight(path, Adesk::kFalse);
	return PyWarnIfError(es, L"PyDbEntity::popHighlight");
}

// --- 拉伸点 ---
py::tuple PyDbEntity::getStretchPoints() const {
	if (!impObj()) return py::tuple();
	AcGePoint3dArray pts;
	if (!PyWarnIfError(impObj()->getStretchPoints(pts), L"AcDbEntity::getStretchPoints")) return py::make_tuple(py::list());
	py::list out;
	for (int i = 0; i < pts.length(); ++i) out.append(pts[i]);
	return py::make_tuple(out);
}

void PyDbEntity::moveStretchPointsAt(const std::vector<int>& indices, const AcGeVector3d& offset) {
	if (!impObj()) return;
	AcDbIntArray idx;
	for (int i : indices) idx.append(i);
	PyWarnIfError(impObj()->moveStretchPointsAt(idx, offset), L"AcDbEntity::moveStretchPointsAt");
}

void bind_PyDbEntity(pym& m) {
	py::class_<PyDbEntity, PyDbObject>(m, "PyDbEntity")
		.def(py::init<AcDbEntity*>(), py::arg("object"))
		.def("colorIndex", &PyDbEntity::colorIndex)
		.def("setColorIndex", &PyDbEntity::setColorIndex, py::arg("colorIndex"), py::arg("doSubents") = true)
		.def("layer", &PyDbEntity::layer)
		.def("layerId", &PyDbEntity::layerId)
		.def("setLayer", &PyDbEntity::setLayer, py::arg("name"), py::arg("doSubents") = true)
		.def("setLayerId", &PyDbEntity::setLayerId, py::arg("layerId"), py::arg("doSubents") = true,
			"按图层 ObjectId（oldId uint64）设置图层，绕开图层名编码问题")
		.def("linetype", &PyDbEntity::linetype)
		.def("linetypeId", &PyDbEntity::linetypeId)
		.def("setLinetype", &PyDbEntity::setLinetype, py::arg("name"), py::arg("doSubents") = true)
		.def("lineWeight", &PyDbEntity::lineWeight)
		.def("setLineWeight", &PyDbEntity::setLineWeight, py::arg("newVal"), py::arg("doSubents") = true)
		.def("visibility", &PyDbEntity::visibility)
		.def("setVisibility", &PyDbEntity::setVisibility, py::arg("newVal"), py::arg("doSubents") = true)
		.def("linetypeScale", &PyDbEntity::linetypeScale)
		.def("setLinetypeScale", &PyDbEntity::setLinetypeScale, py::arg("newval"), py::arg("doSubents") = true)
		.def("getGeomExtents", &PyDbEntity::getGeomExtents, "返回 (minPoint, maxPoint) 元组；获取失败返回 None")
		.def("transformBy", &PyDbEntity::transformBy, py::arg("xform"))
		.def("intersectWith", &PyDbEntity::intersectWith, py::arg("other"), py::arg("intType"))
		// ---- 高亮 / 反选 ----
		.def("highlight", [](PyDbEntity& self, bool doIt) {
			return PyWarnIfError(self.impObj()->highlight(AcDbFullSubentPath(), doIt ? Adesk::kTrue : Adesk::kFalse), L"highlight");
		}, py::arg("do_it") = true, "高亮实体")
		.def("unhighlight", [](PyDbEntity& self) {
			return PyWarnIfError(self.impObj()->unhighlight(AcDbFullSubentPath(), Adesk::kTrue), L"unhighlight");
		}, "取消高亮")
		// 变换副本（游离对象，需 appendEntity 入库）
		.def("getTransformedCopy", [](PyDbEntity& self, const AcGeMatrix3d& xform) -> PyDbEntity {
			AcDbEntity* p = nullptr;
			if (!self.impObj()) return PyDbEntity(nullptr);
			if (self.impObj()->getTransformedCopy(xform, p) != Acad::eOk) p = nullptr;
			return PyDbEntity(p);
		}, py::arg("xform"), "返回变换后的实体副本（未入库）")
		// ---- 颜色 / 透明度 ----
		.def("color", &PyDbEntity::color, "返回真彩色对象 PyColor")
		.def("setColorByRgb", &PyDbEntity::setColorByRgb,
			py::arg("r"), py::arg("g"), py::arg("b"), py::arg("doSubents") = true,
			"按真彩色 RGB 设置颜色（分量夹紧 0-255）")
		.def("entityColor", &PyDbEntity::entityColor, "返回有效实体颜色 RGB (r,g,b)")
		.def("colorRGB", &PyDbEntity::colorRGB, "返回存储的真彩色 RGB (r,g,b)")
		.def("displayColor", &PyDbEntity::displayColor,
			"返回真实显示色 RGB (r,g,b)：解析 ByLayer(随图层)、ByBlock(随块递归)、ACI、前景色")
		.def("isColorVisible", &PyDbEntity::isColorVisible,
			"颜色 257(None)/图层关闭或冻结/实体不可见时返回 False")
		.def("transparency", &PyDbEntity::transparency, "返回透明度 alpha 0=全透明 255=不透明")
		.def("setTransparency", &PyDbEntity::setTransparency, py::arg("alpha"), py::arg("doSubents") = true, "设置透明度 alpha 0-255")
		// ---- 包围盒 / 炸开 ----
		.def("bounds", &PyDbEntity::bounds, "返回包围盒 (minPoint, maxPoint)")
		.def("explode", [](const PyDbEntity& self) -> std::vector<PyDbEntity> {
			std::vector<PyDbEntity> out;
			AcDbVoidPtrArray arr;
			if (!self.impObj()) return out;
			if (self.impObj()->explode(arr) != Acad::eOk) return out;
			for (int i = 0; i < arr.length(); ++i) out.emplace_back(static_cast<AcDbEntity*>(arr[i]));
			return out;
		}, "炸开实体，返回新实体列表（未入库，需 appendEntity）")
		// ---- 平面性 ----
		.def("getPlane", &PyDbEntity::getPlane, "返回 (plane, planarity枚举)")
		.def("isPlanar", &PyDbEntity::isPlanar)
		.def("isContentSnappable", &PyDbEntity::isContentSnappable)
		// ---- 材质 ----
		.def("material", &PyDbEntity::material)
		.def("materialId", &PyDbEntity::materialId)
		.def("setMaterial", &PyDbEntity::setMaterial, py::arg("name"), py::arg("doSubents") = true)
		// ---- 数据库默认值 / 属性复制 ----
		.def("setDatabaseDefaults", &PyDbEntity::setDatabaseDefaults)
		.def("setPropertiesFrom", &PyDbEntity::setPropertiesFrom, py::arg("src"), py::arg("doSubents") = true)
		// ---- 阴影 ----
		.def("castShadows", &PyDbEntity::castShadows)
		.def("setCastShadows", &PyDbEntity::setCastShadows, py::arg("newVal"))
		.def("receiveShadows", &PyDbEntity::receiveShadows)
		.def("setReceiveShadows", &PyDbEntity::setReceiveShadows, py::arg("newVal"))
		// ---- 其它显示 / 图形状态 ----
		.def("collisionType", &PyDbEntity::collisionType)
		.def("recordGraphicsModified", &PyDbEntity::recordGraphicsModified, py::arg("setModified") = true)
		.def("visualStyleId", &PyDbEntity::visualStyleId)
		.def("setVisualStyle", &PyDbEntity::setVisualStyle, py::arg("visualStyleId"), py::arg("doSubents") = true)
		.def("blockId", &PyDbEntity::blockId)
		.def("getEcs", &PyDbEntity::getEcs, "返回实体坐标系(ECS)到 WCS 的变换矩阵")
		// ---- 打印样式 ----
		.def("plotStyleName", &PyDbEntity::plotStyleName)
		.def("setPlotStyleName", &PyDbEntity::setPlotStyleName, py::arg("name"))
		// ---- 其它 ----
		.def("list", &PyDbEntity::list, "等价 CAD LIST 命令，打印实体信息")
		.def("draw", &PyDbEntity::draw, "绘制到当前视图")
		.def("getGripPoints", &PyDbEntity::getGripPoints, "返回 (gripPoints, osnapModes, geomIds)")
		.def("moveGripPointsAt", &PyDbEntity::moveGripPointsAt, py::arg("indices"), py::arg("offset"))
		// --- 高亮 ---
		.def("pushHighlight", &PyDbEntity::pushHighlight, py::arg("style") = 0, "高亮显示")
		.def("popHighlight", &PyDbEntity::popHighlight, "取消高亮")
		// --- 拉伸点 ---
		.def("getStretchPoints", &PyDbEntity::getStretchPoints, "获取拉伸点")
		.def("moveStretchPointsAt", &PyDbEntity::moveStretchPointsAt, py::arg("indices"), py::arg("offset"), "移动拉伸点")
		// ---- intersectWith 带投影平面的重载 ----
		.def("intersectWith", [](const PyDbEntity& self, const PyDbEntity& other, int intType,
				const AcGePlane& projPlane, int thisGsMarker, int otherGsMarker) -> std::vector<AcGePoint3d> {
			std::vector<AcGePoint3d> out;
			AcGePoint3dArray pts;
			if (!self.impObj() || !other.impObj()) return out;
			if (self.impObj()->intersectWith(other.impObj(), static_cast<AcDb::Intersect>(intType), projPlane, pts,
					static_cast<Adesk::GsMarker>(thisGsMarker), static_cast<Adesk::GsMarker>(otherGsMarker)) != Acad::eOk) return out;
			for (int i = 0; i < pts.length(); ++i) out.push_back(pts[i]);
			return out;
		}, py::arg("other"), py::arg("intType"), py::arg("proj_plane"), py::arg("thisGsMarker") = 0, py::arg("otherGsMarker") = 0)
		// ---- boundingBoxIntersectWith 两个重载 ----
		.def("boundingBoxIntersectWith", [](const PyDbEntity& self, const PyDbEntity& other, int intType,
				const AcGePlane& projPlane, int thisGsMarker, int otherGsMarker) -> std::vector<AcGePoint3d> {
			std::vector<AcGePoint3d> out;
			AcGePoint3dArray pts;
			if (!self.impObj() || !other.impObj()) return out;
			if (self.impObj()->boundingBoxIntersectWith(other.impObj(), static_cast<AcDb::Intersect>(intType), projPlane, pts,
					static_cast<Adesk::GsMarker>(thisGsMarker), static_cast<Adesk::GsMarker>(otherGsMarker)) != Acad::eOk) return out;
			for (int i = 0; i < pts.length(); ++i) out.push_back(pts[i]);
			return out;
		}, py::arg("other"), py::arg("intType"), py::arg("proj_plane"), py::arg("thisGsMarker") = 0, py::arg("otherGsMarker") = 0)
		.def("boundingBoxIntersectWith", [](const PyDbEntity& self, const PyDbEntity& other, int intType,
				int thisGsMarker, int otherGsMarker) -> std::vector<AcGePoint3d> {
			std::vector<AcGePoint3d> out;
			AcGePoint3dArray pts;
			if (!self.impObj() || !other.impObj()) return out;
			if (self.impObj()->boundingBoxIntersectWith(other.impObj(), static_cast<AcDb::Intersect>(intType), pts,
					static_cast<Adesk::GsMarker>(thisGsMarker), static_cast<Adesk::GsMarker>(otherGsMarker)) != Acad::eOk) return out;
			for (int i = 0; i < pts.length(); ++i) out.push_back(pts[i]);
			return out;
		}, py::arg("other"), py::arg("intType"), py::arg("thisGsMarker") = 0, py::arg("otherGsMarker") = 0)
		PyWrapper_BIND(DbEntity)
		;
}
