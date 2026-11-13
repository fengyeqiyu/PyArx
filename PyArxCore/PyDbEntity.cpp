#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbEntity.h"

PyWrapper_Cpp(DbEntity)

unsigned short PyDbEntity::colorIndex() const { return impObj()->colorIndex(); }
bool  PyDbEntity::setColorIndex(unsigned short colorIndex, bool doSubents) {
	return PyWarnIfError(impObj()->setColorIndex(static_cast<Adesk::UInt16>(colorIndex), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setColorIndex");
}

AcString PyDbEntity::layer() const { AcString s; if(!PyWarnIfError(impObj()->layer(s), L"AcDbEntity::layer")) return AcString(); return s; }
uint64_t PyDbEntity::layerId() const { return static_cast<uint64_t>(impObj()->layerId().asOldId()); }
bool  PyDbEntity::setLayer(const AcString& name, bool doSubents) {
	return PyWarnIfError(impObj()->setLayer(name.kwszPtr(), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setLayer");
}

AcString PyDbEntity::linetype() const { AcString s; if(!PyWarnIfError(impObj()->linetype(s), L"AcDbEntity::linetype")) return AcString(); return s; }
uint64_t PyDbEntity::linetypeId() const { return static_cast<uint64_t>(impObj()->linetypeId().asOldId()); }
bool  PyDbEntity::setLinetype(const AcString& name, bool doSubents) {
	return PyWarnIfError(impObj()->setLinetype(name.kwszPtr(), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setLinetype");
}

int PyDbEntity::lineWeight() const { return static_cast<int>(impObj()->lineWeight()); }
bool  PyDbEntity::setLineWeight(int newVal, bool doSubents) {
	return PyWarnIfError(impObj()->setLineWeight(static_cast<AcDb::LineWeight>(newVal), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setLineWeight");
}

int PyDbEntity::visibility() const { return static_cast<int>(impObj()->visibility()); }
bool  PyDbEntity::setVisibility(int newVal, bool doSubents) {
	return PyWarnIfError(impObj()->setVisibility(static_cast<AcDb::Visibility>(newVal), doSubents), L"AcDbEntity::setVisibility");
}

double PyDbEntity::linetypeScale() const { return impObj()->linetypeScale(); }
bool  PyDbEntity::setLinetypeScale(double newval, bool doSubents) {
	return PyWarnIfError(impObj()->setLinetypeScale(newval, doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setLinetypeScale");
}

py::object PyDbEntity::getGeomExtents() const {
	AcDbExtents ext;
	if(!PyWarnIfError(impObj()->getGeomExtents(ext), L"AcDbEntity::getGeomExtents")) return py::none();
	return py::make_tuple(ext.minPoint(), ext.maxPoint());
}

bool  PyDbEntity::transformBy(const AcGeMatrix3d& xform) {
	return PyWarnIfError(impObj()->transformBy(xform), L"AcDbEntity::transformBy");
}

py::list PyDbEntity::intersectWith(const PyDbEntity& other, int intType) const {
	py::list out;
	AcGePoint3dArray pts;
	if(!PyWarnIfError(impObj()->intersectWith(other.impObj(), static_cast<AcDb::Intersect>(intType), pts), L"AcDbEntity::intersectWith")) return out;
	for(int i = 0; i < pts.length(); ++i) out.append(pts[i]);
	return out;
}

// ---- 颜色 / 透明度（AcCm* 未单独绑定，提取为基本类型）----
PyColor PyDbEntity::color() const {
	return PyColor(impObj()->color());
}
bool PyDbEntity::setColor(const PyColor& color, bool doSubents) {
	return PyWarnIfError(impObj()->setColor(color.m_color, doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setColor");
}
std::tuple<int,int,int> PyDbEntity::entityColor() const {
	AcCmEntityColor c = impObj()->entityColor();
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
std::tuple<int,int,int> PyDbEntity::colorRGB() const {
	AcCmColor c = impObj()->color();
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
int  PyDbEntity::transparency() const { return (int)impObj()->transparency().alpha(); }
bool PyDbEntity::setTransparency(int alpha, bool doSubents) {
	AcCmTransparency t(static_cast<Adesk::UInt8>(alpha));
	return PyWarnIfError(impObj()->setTransparency(t, doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setTransparency");
}

// ---- 包围盒 ----
std::tuple<AcGePoint3d,AcGePoint3d> PyDbEntity::bounds() const {
	AcDbExtents ext;
	if(!impObj()->bounds(ext)) return std::make_tuple(AcGePoint3d(), AcGePoint3d());
	return std::make_tuple(ext.minPoint(), ext.maxPoint());
}

// ---- 平面性 ----
std::tuple<AcGePlane,int> PyDbEntity::getPlane() const {
	AcGePlane pl; AcDb::Planarity pa;
	if(!PyWarnIfError(impObj()->getPlane(pl, pa), L"AcDbEntity::getPlane")) return std::make_tuple(AcGePlane(), (int)-1);
	return std::make_tuple(pl, (int)pa);
}
bool PyDbEntity::isPlanar() const { return impObj()->isPlanar() != Adesk::kFalse; }
bool PyDbEntity::isContentSnappable() const { return impObj()->isContentSnappable(); }

// ---- 材质 ----
AcString PyDbEntity::material() const { AcString s; if(!PyWarnIfError(impObj()->material(s), L"AcDbEntity::material")) return AcString(); return s; }
uint64_t PyDbEntity::materialId() const { return static_cast<uint64_t>(impObj()->materialId().asOldId()); }
bool  PyDbEntity::setMaterial(const AcString& name, bool doSubents) {
	return PyWarnIfError(impObj()->setMaterial(name.kwszPtr(), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setMaterial");
}

// ---- 数据库默认值 / 属性复制 ----
void PyDbEntity::setDatabaseDefaults() { impObj()->setDatabaseDefaults(); }
bool PyDbEntity::setPropertiesFrom(const PyDbEntity& src, bool doSubents) {
	return PyWarnIfError(impObj()->setPropertiesFrom(src.impObj(), doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setPropertiesFrom");
}

// ---- 阴影 ----
bool PyDbEntity::castShadows() const { return impObj()->castShadows(); }
bool PyDbEntity::setCastShadows(bool newVal) { impObj()->setCastShadows(newVal); return true; }
bool PyDbEntity::receiveShadows() const { return impObj()->receiveShadows(); }
bool PyDbEntity::setReceiveShadows(bool newVal) { impObj()->setReceiveShadows(newVal); return true; }

// ---- 其它显示 / 图形状态 ----
int  PyDbEntity::collisionType() const { return static_cast<int>(impObj()->collisionType()); }
void PyDbEntity::recordGraphicsModified(bool setModified) { impObj()->recordGraphicsModified(setModified ? Adesk::kTrue : Adesk::kFalse); }
uint64_t PyDbEntity::visualStyleId() const { return static_cast<uint64_t>(impObj()->visualStyleId().asOldId()); }
bool PyDbEntity::setVisualStyle(uint64_t visualStyleId, bool doSubents) {
	AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::IntDbId>(visualStyleId));
	return PyWarnIfError(impObj()->setVisualStyle(oid, AcDbEntity::kFullVisualStyle, doSubents ? Adesk::kTrue : Adesk::kFalse), L"AcDbEntity::setVisualStyle");
}
uint64_t PyDbEntity::blockId() const { return static_cast<uint64_t>(impObj()->blockId().asOldId()); }
AcGeMatrix3d PyDbEntity::getEcs() const { AcGeMatrix3d m; impObj()->getEcs(m); return m; }

// ---- 打印样式 ----
AcString PyDbEntity::plotStyleName() const { AcString s; impObj()->plotStyleName(s); return s; }
bool PyDbEntity::setPlotStyleName(const AcString& name) {
	return PyWarnIfError(impObj()->setPlotStyleName(name.kwszPtr()), L"AcDbEntity::setPlotStyleName");
}

// ---- 其它 ----
void PyDbEntity::list() const { impObj()->list(); }
void PyDbEntity::draw() const { impObj()->draw(); }

py::tuple PyDbEntity::getGripPoints() const {
	AcGePoint3dArray gripPoints;
	AcDbIntArray osnapModes, geomIds;
	impObj()->getGripPoints(gripPoints, osnapModes, geomIds);
	py::list pyGrip, pyOsnap, pyGeom;
	for (int i = 0; i < gripPoints.length(); i++) pyGrip.append(gripPoints[i]);
	for (int i = 0; i < osnapModes.length(); i++) pyOsnap.append(osnapModes[i]);
	for (int i = 0; i < geomIds.length(); i++) pyGeom.append(geomIds[i]);
	return py::make_tuple(pyGrip, pyOsnap, pyGeom);
}

void PyDbEntity::moveGripPointsAt(const std::vector<int>& indices, const AcGeVector3d& offset) {
	AcDbIntArray idx;
	for (int i : indices) idx.append(i);
	impObj()->moveGripPointsAt(idx, offset);
}

// --- 高亮 ---
bool PyDbEntity::pushHighlight(int style) {
	AcDbFullSubentPath path;
	return impObj()->highlight(path, Adesk::kTrue) == Acad::eOk;
}

bool PyDbEntity::popHighlight() {
	AcDbFullSubentPath path;
	return impObj()->highlight(path, Adesk::kFalse) == Acad::eOk;
}

// --- 拉伸点 ---
py::tuple PyDbEntity::getStretchPoints() const {
	AcGePoint3dArray pts;
	if (impObj()->getStretchPoints(pts) != Acad::eOk) return py::make_tuple(py::list());
	py::list out;
	for (int i = 0; i < pts.length(); ++i) out.append(pts[i]);
	return py::make_tuple(out);
}

void PyDbEntity::moveStretchPointsAt(const std::vector<int>& indices, const AcGeVector3d& offset) {
	AcDbIntArray idx;
	for (int i : indices) idx.append(i);
	impObj()->moveStretchPointsAt(idx, offset);
}

void bind_PyDbEntity(pym& m) {
	py::class_<PyDbEntity, PyDbObject>(m, "PyDbEntity")
		.def(py::init<AcDbEntity*>(), py::arg("object"))
		.def("colorIndex", &PyDbEntity::colorIndex)
		.def("setColorIndex", &PyDbEntity::setColorIndex, py::arg("colorIndex"), py::arg("doSubents") = true)
		.def("layer", &PyDbEntity::layer)
		.def("layerId", &PyDbEntity::layerId)
		.def("setLayer", &PyDbEntity::setLayer, py::arg("name"), py::arg("doSubents") = true)
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
		// ---- 真彩色 RGB（直接构造 AcCmColor，无需单独绑 AcCmColor 类）----
		.def("setRGB", [](PyDbEntity& self, int r, int g, int b) {
			AcCmColor c;
			c.setRGB(static_cast<Adesk::UInt8>(r), static_cast<Adesk::UInt8>(g), static_cast<Adesk::UInt8>(b));
			return PyWarnIfError(self.impObj()->setColor(c), L"setColor");
		}, py::arg("r"), py::arg("g"), py::arg("b"), "设置真彩色 0-255")
		// 变换副本（游离对象，需 appendEntity 入库）
		.def("getTransformedCopy", [](PyDbEntity& self, const AcGeMatrix3d& xform) -> PyDbEntity {
			AcDbEntity* p = nullptr;
			if (self.impObj()->getTransformedCopy(xform, p) != Acad::eOk) p = nullptr;
			return PyDbEntity(p);
		}, py::arg("xform"), "返回变换后的实体副本（未入库）")
		// ---- 颜色 / 透明度 ----
		.def("color", &PyDbEntity::color, "返回真彩色对象 PyColor")
		.def("setColor", &PyDbEntity::setColor, py::arg("color"), py::arg("doSubents") = true, "用 PyColor 对象设置颜色")
		.def("entityColor", &PyDbEntity::entityColor, "返回有效实体颜色 RGB (r,g,b)")
		.def("colorRGB", &PyDbEntity::colorRGB, "返回存储的真彩色 RGB (r,g,b)")
		.def("transparency", &PyDbEntity::transparency, "返回透明度 alpha 0=全透明 255=不透明")
		.def("setTransparency", &PyDbEntity::setTransparency, py::arg("alpha"), py::arg("doSubents") = true, "设置透明度 alpha 0-255")
		// ---- 包围盒 / 炸开 ----
		.def("bounds", &PyDbEntity::bounds, "返回包围盒 (minPoint, maxPoint)")
		.def("explode", [](const PyDbEntity& self) -> std::vector<PyDbEntity> {
			std::vector<PyDbEntity> out;
			AcDbVoidPtrArray arr;
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
			if (self.impObj()->boundingBoxIntersectWith(other.impObj(), static_cast<AcDb::Intersect>(intType), projPlane, pts,
					static_cast<Adesk::GsMarker>(thisGsMarker), static_cast<Adesk::GsMarker>(otherGsMarker)) != Acad::eOk) return out;
			for (int i = 0; i < pts.length(); ++i) out.push_back(pts[i]);
			return out;
		}, py::arg("other"), py::arg("intType"), py::arg("proj_plane"), py::arg("thisGsMarker") = 0, py::arg("otherGsMarker") = 0)
		.def("boundingBoxIntersectWith", [](const PyDbEntity& self, const PyDbEntity& other, int intType,
				int thisGsMarker, int otherGsMarker) -> std::vector<AcGePoint3d> {
			std::vector<AcGePoint3d> out;
			AcGePoint3dArray pts;
			if (self.impObj()->boundingBoxIntersectWith(other.impObj(), static_cast<AcDb::Intersect>(intType), pts,
					static_cast<Adesk::GsMarker>(thisGsMarker), static_cast<Adesk::GsMarker>(otherGsMarker)) != Acad::eOk) return out;
			for (int i = 0; i < pts.length(); ++i) out.push_back(pts[i]);
			return out;
		}, py::arg("other"), py::arg("intType"), py::arg("thisGsMarker") = 0, py::arg("otherGsMarker") = 0)
		PyWrapper_BIND(DbEntity)
		;
}
