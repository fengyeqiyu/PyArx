// PyGiBinder.cpp —— PyArx.Gi 子模块绑定：AcGi 图形接口类。
// 依赖 PyRxObject / AcDbObjectId / PyDbEntity（均注册于 PyArx.Db 子模块），
// 故 bind_PyGi 须在 PyArx.Db 之后由 PyArxBinder.cpp 调用。
//
// 2026-11-15 对齐 acgi.h / acgidefs.h / acgitransient.h 全量补齐：
//   - 枚举：AcGiFillType / RegenType / ArcType / ImageOrg / Visibility /
//           OrientationType / DeviationType / 三种 TransformBehavior /
//           TransparencyMode / DrawFlags / ShadowFlags / SelectionFlags /
//           HighlightProperty / AcGiLineType
//   - 值类型：AcCmEntityColor / AcCmTransparency / AcGiVariant / AcGiMapper /
//             AcGiPolyline / AcGiImageBGRA32 / AcGiFill 族（见各自 *.cpp）
//   - AcGiViewport / AcGiTransientManager / Python 自绘 drawable 桥
//   - AcGiGeometry 全图元 / AcGiViewportGeometry 的 eye-dc 版
// 已补齐（第二轮）：AcGiContext / AcGiGdiDrawObject+ownerDraw(HDC) / AcGiDrawStream /
//   AcGiEnvironment / LightTraits / VisualStyle / Material / polyPolygon /
//   AcGiEdgeData / AcGiFaceData / AcGiVertexData。
// 仍不导出（SDK 内部分或依赖尚未绑定类型）：
//   polyPolyline（AcGiPolyline 连续数组 + 拷贝语义不安全）、
//   edge(AcArray<AcGeCurve2d*>)、AcGiDrawStream::bounds(AcDbExtents) /
//   serializeIn/Out(IAcRead/WriteStream)。
#include "stdafx.h"
#include "PyGiBinder.h"
#include "PyGiDrawable.h"
#include "PyGiSubEntityTraits.h"
#include "PyGiWorldGeometry.h"
#include "PyGiTextStyle.h"
#include "PyGiTextEngine.h"
#include "PyGiColor.h"
#include "PyGiFill.h"
#include "PyGiPolyline.h"
#include "PyGiImage.h"
#include "PyGiViewport.h"
#include "PyGiTransient.h"
#include "PyGiMeshData.h"
#include "PyGiContextGdi.h"
#include "PyGiStreamStyle.h"
#include "PyGiMaterialLight.h"
#include "PyGiTypes.h"
#include "PyRxObject.h"
#include "PyDbTypeCaster.h"
#include "PyDbEntity.h"
#include "PyDbPolyline.h"
#include "PyErrorStatusException.h"

// ===========================================================================
// AcGi 枚举（acgidefs.h / acgi.h）
// ===========================================================================
void bind_PyGiEnums(pym& m) {
	py::enum_<AcGiFillType>(m, "AcGiFillType")
		.value("kAcGiFillAlways", kAcGiFillAlways)
		.value("kAcGiFillNever", kAcGiFillNever);

	py::enum_<AcGiRegenType>(m, "AcGiRegenType")
		.value("eAcGiRegenTypeInvalid", eAcGiRegenTypeInvalid)
		.value("kAcGiStandardDisplay", kAcGiStandardDisplay)
		.value("kAcGiHideOrShadeCommand", kAcGiHideOrShadeCommand)
		.value("kAcGiRenderCommand", kAcGiRenderCommand)
		.value("kAcGiShadedDisplay", kAcGiShadedDisplay) // PyRx 同义名（= kAcGiRenderCommand）
		.value("kAcGiForExplode", kAcGiForExplode)
		.value("kAcGiSaveWorldDrawForProxy", kAcGiSaveWorldDrawForProxy);

	py::enum_<AcGiArcType>(m, "AcGiArcType")
		.value("kAcGiArcSimple", kAcGiArcSimple)
		.value("kAcGiArcSector", kAcGiArcSector)
		.value("kAcGiArcChord", kAcGiArcChord);

	py::enum_<AcGiVisibility>(m, "AcGiVisibility")
		.value("kAcGiInvisible", kAcGiInvisible)
		.value("kAcGiVisible", kAcGiVisible)
		.value("kAcGiSilhouette", kAcGiSilhouette);

	py::enum_<AcGiOrientationType>(m, "AcGiOrientationType")
		.value("kAcGiCounterClockwise", kAcGiCounterClockwise)
		.value("kAcGiNoOrientation", kAcGiNoOrientation)
		.value("kAcGiClockwise", kAcGiClockwise);

	py::enum_<AcGiDeviationType>(m, "AcGiDeviationType")
		.value("kAcGiMaxDevForCircle", kAcGiMaxDevForCircle)
		.value("kAcGiMaxDevForCurve", kAcGiMaxDevForCurve)
		.value("kAcGiMaxDevForBoundary", kAcGiMaxDevForBoundary)
		.value("kAcGiMaxDevForIsoline", kAcGiMaxDevForIsoline)
		.value("kAcGiMaxDevForFacet", kAcGiMaxDevForFacet);

	py::enum_<AcGiImageOrg>(m, "AcGiImageOrg")
		.value("kAcGiBitonal", kAcGiBitonal)
		.value("kAcGiPalette", kAcGiPalette)
		.value("kAcGiGray", kAcGiGray)
		.value("kAcGiRGBA", kAcGiRGBA)
		.value("kAcGiBGRA", kAcGiBGRA)
		.value("kAcGiARGB", kAcGiARGB)
		.value("kAcGiABGR", kAcGiABGR)
		.value("kAcGiBGR", kAcGiBGR)
		.value("kAcGiRGB", kAcGiRGB);

	py::enum_<AcGiImageOrient>(m, "AcGiImageOrient")
		.value("kAcGiXLeftToRightTopFirst", kAcGiXLeftToRightTopFirst)
		.value("kAcGiXLeftToRightBottomFirst", kAcGiXLeftToRightBottomFirst)
		.value("kAcGiXRightToLeftTopFirst", kAcGiXRightToLeftTopFirst)
		.value("kAcGiXRightToLeftBottomFirst", kAcGiXRightToLeftBottomFirst)
		.value("kAcGiYTopToBottomLeftFirst", kAcGiYTopToBottomLeftFirst)
		.value("kAcGiYTopToBottomRightFirst", kAcGiYTopToBottomRightFirst)
		.value("kAcGiYBottomToTopLeftFirst", kAcGiYBottomToTopLeftFirst)
		.value("kAcGiYBottomToTopRightFirst", kAcGiYBottomToTopRightFirst);

	py::enum_<AcGiPositionTransformBehavior>(m, "AcGiPositionTransformBehavior")
		.value("kAcGiWorldPosition", kAcGiWorldPosition)
		.value("kAcGiViewportPosition", kAcGiViewportPosition)
		.value("kAcGiScreenPosition", kAcGiScreenPosition)
		.value("kAcGiScreenLocalOriginPosition", kAcGiScreenLocalOriginPosition)
		.value("kAcGiWorldWithScreenOffsetPosition", kAcGiWorldWithScreenOffsetPosition);

	py::enum_<AcGiScaleTransformBehavior>(m, "AcGiScaleTransformBehavior")
		.value("kAcGiWorldScale", kAcGiWorldScale)
		.value("kAcGiViewportScale", kAcGiViewportScale)
		.value("kAcGiScreenScale", kAcGiScreenScale)
		.value("kAcGiViewportLocalOriginScale", kAcGiViewportLocalOriginScale)
		.value("kAcGiScreenLocalOriginScale", kAcGiScreenLocalOriginScale);

	py::enum_<AcGiOrientationTransformBehavior>(m, "AcGiOrientationTransformBehavior")
		.value("kAcGiWorldOrientation", kAcGiWorldOrientation)
		.value("kAcGiScreenOrientation", kAcGiScreenOrientation)
		.value("kAcGiZAxisOrientation", kAcGiZAxisOrientation);

	py::enum_<AcGiGeometry::TransparencyMode>(m, "AcGiTransparencyMode")
		.value("kTransparencyOff", AcGiGeometry::kTransparencyOff)
		.value("kTransparency1Bit", AcGiGeometry::kTransparency1Bit)
		.value("kTransparency8Bit", AcGiGeometry::kTransparency8Bit);

	py::enum_<AcGiSubEntityTraits::DrawFlags>(m, "AcGiDrawFlags", py::arithmetic())
		.value("kNoDrawFlags", AcGiSubEntityTraits::kNoDrawFlags)
		.value("kDrawBackfaces", AcGiSubEntityTraits::kDrawBackfaces)
		.value("kDrawHatchGroup", AcGiSubEntityTraits::kDrawHatchGroup)
		.value("kDrawFrontfacesOnly", AcGiSubEntityTraits::kDrawFrontfacesOnly)
		.value("kDrawGradientFill", AcGiSubEntityTraits::kDrawGradientFill)
		.value("kDrawSolidFill", AcGiSubEntityTraits::kDrawSolidFill)
		.value("kDrawNoLineWeight", AcGiSubEntityTraits::kDrawNoLineWeight)
		.value("kDrawNoOptimization", AcGiSubEntityTraits::kDrawNoOptimization)
		.value("kDrawUseAcGiEntityForDgnLineType", AcGiSubEntityTraits::kDrawUseAcGiEntityForDgnLineType)
		.value("kDrawFillTextBoundaryStart", AcGiSubEntityTraits::kDrawFillTextBoundaryStart)
		.value("kDrawFillTextBoundaryEnd", AcGiSubEntityTraits::kDrawFillTextBoundaryEnd)
		.value("kDrawFillSelectionWindow", AcGiSubEntityTraits::kDrawFillSelectionWindow)
		.value("kDrawNoForceByLayer", AcGiSubEntityTraits::kDrawNoForceByLayer)
		.value("kDrawIsInWCS", AcGiSubEntityTraits::kDrawIsInWCS)
		.value("kDrawNoImageFrame", AcGiSubEntityTraits::kDrawNoImageFrame);

	py::enum_<AcGiSubEntityTraits::ShadowFlags>(m, "AcGiShadowFlags")
		.value("kShadowsCastAndReceive", AcGiSubEntityTraits::kShadowsCastAndReceive)
		.value("kShadowsDoesNotCast", AcGiSubEntityTraits::kShadowsDoesNotCast)
		.value("kShadowsDoesNotReceive", AcGiSubEntityTraits::kShadowsDoesNotReceive)
		.value("kShadowsIgnore", AcGiSubEntityTraits::kShadowsIgnore)
		.value("kNoDrawFlags", static_cast<AcGiSubEntityTraits::ShadowFlags>(0)); // PyRx 同义名;

	py::enum_<AcGiSubEntityTraits::SelectionFlags>(m, "AcGiSelectionFlags")
		.value("kNoSelectionFlags", AcGiSubEntityTraits::kNoSelectionFlags)
		.value("kSelectionIgnore", AcGiSubEntityTraits::kSelectionIgnore);

	py::enum_<AcGiDrawableTraits::HighlightProperty>(m, "AcGiHighlightProperty")
		.value("kVertexRolloverHighlightSize", AcGiDrawableTraits::kVertexRolloverHighlightSize);

	py::enum_<AcGiLineType>(m, "AcGiLineType")
		.value("kLineTypeSolid", kLineTypeSolid)
		.value("kDashed", kDashed)
		.value("kDotted", kDotted)
		.value("kDash_Dot", kDash_Dot)
		.value("kShort_Dash", kShort_Dash)
		.value("kMedium_Dash", kMedium_Dash)
		.value("kLong_Dash", kLong_Dash)
		.value("kMedium_Long_Dash", kMedium_Long_Dash)
		.value("kLong_Dash_Short_Dash", kLong_Dash_Short_Dash)
		.value("kLong_Dash_Dot", kLong_Dash_Dot)
		.value("kSparse_Dot", kSparse_Dot)
		.value("kISO_Dash", kISO_Dash)
		.value("kISO_Dash_Space", kISO_Dash_Space)
		.value("kISO_Long_Dash_Dot", kISO_Long_Dash_Dot)
		.value("kISO_Dot", kISO_Dot)
		.value("kISO_Long_Dash_Short_Dash", kISO_Long_Dash_Short_Dash)
		.value("kISO_Dash_Dot", kISO_Dash_Dot)
		.value("kLineTypeNone", kLineTypeNone)
		.value("kDense_Dot", kDense_Dot);
}

// ===========================================================================
// PyGiSubEntityTraits —— 子实体外观特性
// ===========================================================================
void bind_PyGiSubEntityTraits(pym& m) {
	auto cls = py::class_<PyGiSubEntityTraits, PyRxObject>(m, "PyGiSubEntityTraits");
	cls
		PyWrapper_BIND(GiSubEntityTraits)
		.def("setColor", [](PyGiSubEntityTraits& s, int color) { s.setColor(static_cast<Adesk::UInt16>(color)); }, py::arg("color"))
		.def("color", &PyGiSubEntityTraits::color)
		.def("setTrueColor", [](PyGiSubEntityTraits& s, const AcCmEntityColor& c) { s.setTrueColor(c); }, py::arg("color"))
		.def("trueColor", &PyGiSubEntityTraits::trueColor)
		.def("setLayer", &PyGiSubEntityTraits::setLayer, py::arg("layerId"))
		.def("layerId", &PyGiSubEntityTraits::layerId)
		.def("setLineType", &PyGiSubEntityTraits::setLineType, py::arg("linetypeId"))
		.def("lineTypeId", &PyGiSubEntityTraits::lineTypeId)
		.def("setSelectionMarker", [](PyGiSubEntityTraits& s, int markerId) { s.setSelectionMarker(static_cast<Adesk::LongPtr>(markerId)); }, py::arg("markerId"))
		.def("setFillType", [](PyGiSubEntityTraits& s, int t) { s.setFillType(static_cast<AcGiFillType>(t)); }, py::arg("fillType"))
		.def("fillType", [](const PyGiSubEntityTraits& s) { return static_cast<int>(s.fillType()); })
		.def("setLineWeight", [](PyGiSubEntityTraits& s, int lw) { s.setLineWeight(static_cast<AcDb::LineWeight>(lw)); }, py::arg("lw"))
		.def("setLineWeight", [](PyGiSubEntityTraits& s, int lw, int index) {
			s.setLineWeight(static_cast<AcDb::LineWeight>(lw), static_cast<char>(index));
		}, py::arg("lw"), py::arg("lw_index"))
		.def("lineWeight", [](const PyGiSubEntityTraits& s) { return static_cast<int>(s.lineWeight()); })
		.def("setLineTypeScale", &PyGiSubEntityTraits::setLineTypeScale, py::arg("dScale") = 1.0)
		.def("lineTypeScale", &PyGiSubEntityTraits::lineTypeScale)
		.def("setThickness", &PyGiSubEntityTraits::setThickness, py::arg("dThickness"))
		.def("thickness", &PyGiSubEntityTraits::thickness)
		.def("setVisualStyle", &PyGiSubEntityTraits::setVisualStyle, py::arg("visualStyleId"))
		.def("visualStyle", &PyGiSubEntityTraits::visualStyle)
		.def("setPlotStyleName", [](PyGiSubEntityTraits& s, int pltType, py::object objid) {
			AcDbObjectId id = objid.is_none() ? AcDbObjectId::kNull : py::cast<AcDbObjectId>(objid);
			s.setPlotStyleName(static_cast<AcDb::PlotStyleNameType>(pltType), id);
		}, py::arg("pltType"), py::arg("objid") = py::none())
		.def("getPlotStyleNameId", [](const PyGiSubEntityTraits& s) {
			AcDbObjectId id = AcDbObjectId::kNull;
			const AcDb::PlotStyleNameType t = s.getPlotStyleNameId(id);
			return py::make_tuple(static_cast<int>(t), id);
		}, "返回 (PlotStyleNameType, ObjectId)")
		.def("setMaterial", &PyGiSubEntityTraits::setMaterial, py::arg("materialId"))
		.def("materialId", &PyGiSubEntityTraits::materialId)
		.def("setMapper", [](PyGiSubEntityTraits& s, const AcGiMapper& mapper) { s.setMapper(mapper); }, py::arg("mapper"))
		.def("mapper", &PyGiSubEntityTraits::mapper)
		.def("setSectionable", &PyGiSubEntityTraits::setSectionable, py::arg("bSectionable"))
		.def("sectionable", &PyGiSubEntityTraits::sectionable)
		.def("setDrawFlags", [](PyGiSubEntityTraits& s, unsigned int flags) {
			return PyWarnIfError(s.setDrawFlags(static_cast<Adesk::UInt32>(flags)), L"setDrawFlags"); }, py::arg("flags"))
		.def("drawFlags", &PyGiSubEntityTraits::drawFlags)
		.def("setShadowFlags", [](PyGiSubEntityTraits& s, int flags) {
			return PyWarnIfError(s.setShadowFlags(static_cast<AcGiSubEntityTraits::ShadowFlags>(flags)), L"setShadowFlags"); }, py::arg("flags"))
		.def("shadowFlags", [](const PyGiSubEntityTraits& s) { return static_cast<int>(s.shadowFlags()); })
		.def("setSelectionGeom", &PyGiSubEntityTraits::setSelectionGeom, py::arg("bSelectionflag"))
		.def("selectionGeom", &PyGiSubEntityTraits::selectionGeom)
		.def("setTransparency", [](PyGiSubEntityTraits& s, const AcCmTransparency& t) { s.setTransparency(t); }, py::arg("transparency"))
		.def("transparency", &PyGiSubEntityTraits::transparency)
		.def("setFill", [](PyGiSubEntityTraits& s, const PyGiFill& fill) { s.setFill(fill); }, py::arg("fill"))
		.def("pushMarkerOverride", [](PyGiSubEntityTraits& s, bool flag, long long markerId) {
			s.pushMarkerOverride(flag, static_cast<Adesk::LongPtr>(markerId));
		}, py::arg("flag"), py::arg("markerId") = -1)
		.def("popMarkerOverride", &PyGiSubEntityTraits::popMarkerOverride)
		.def("clearMarkerOverride", &PyGiSubEntityTraits::clearMarkerOverride);
}

// ===========================================================================
// PyGiDrawableTraits —— 图元特性（继承 PyGiSubEntityTraits）
// ===========================================================================
void bind_PyGiDrawableTraits(pym& m) {
	auto cls = py::class_<PyGiDrawableTraits, PyGiSubEntityTraits>(m, "PyGiDrawableTraits");
	cls
		PyWrapper_BIND(GiDrawableTraits)
		.def("setupForEntity", &PyGiDrawableTraits::setupForEntity, py::arg("pEntity"))
		.def("addLight", &PyGiDrawableTraits::addLight, py::arg("lightId"))
		.def("setLinePattern", &PyGiDrawableTraits::setLinePattern, py::arg("linePattern"))
		.def("linePattern", &PyGiDrawableTraits::linePattern)
		.def("setSelectionFlags", [](PyGiDrawableTraits& t, int flags) {
			return PyWarnIfError(t.setSelectionFlags(static_cast<AcGiSubEntityTraits::SelectionFlags>(flags)), L"setSelectionFlags");
		}, py::arg("flags"))
		.def("selectionFlags", [](const PyGiDrawableTraits& t) { return static_cast<int>(t.selectionFlags()); })
		.def("setHighlightProperty", [](PyGiDrawableTraits& t, int property, AcGiVariant& value) {
			return PyWarnIfError(t.setHighlightProperty(static_cast<AcGiDrawableTraits::HighlightProperty>(property), value), L"setHighlightProperty");
		}, py::arg("property"), py::arg("value"))
		.def("highlightProperty", [](const PyGiDrawableTraits& t, int property) {
			return t.highlightProperty(static_cast<AcGiDrawableTraits::HighlightProperty>(property));
		}, py::arg("property"));
}

// ===========================================================================
// PyGiCommonDraw —— 公共绘制上下文（基类）
// ===========================================================================
void bind_PyGiCommonDraw(pym& m) {
	auto cls = py::class_<PyGiCommonDraw, PyRxObject>(m, "PyGiCommonDraw");
	cls
		PyWrapper_BIND(GiCommonDraw)
		.def("subEntityTraits", &PyGiCommonDraw::subEntityTraits)
		.def("regenType", [](const PyGiCommonDraw& d) { return static_cast<int>(d.regenType()); })
		.def("regenAbort", &PyGiCommonDraw::regenAbort)
		.def("isDragging", &PyGiCommonDraw::isDragging)
		.def("deviation", [](const PyGiCommonDraw& d, int deviationType, py::object pt) {
			return d.deviation(deviationType, AsPoint3d(pt));
		}, py::arg("deviation_type"), py::arg("point"))
		.def("numberOfIsolines", &PyGiCommonDraw::numberOfIsolines)
		.def("secondaryCall", &PyGiCommonDraw::secondaryCall)
		.def("context", &PyGiCommonDraw::context, py::return_value_policy::take_ownership,
			"AcGiContext* context()（未提供上下文时返回 invalid 对象）");
}

// ===========================================================================
// PyGiWorldDraw / PyGiViewportDraw —— 世界/视口绘制上下文
// ===========================================================================
void bind_PyGiWorldDraw(pym& m) {
	auto cls = py::class_<PyGiWorldDraw, PyGiCommonDraw>(m, "PyGiWorldDraw");
	cls
		PyWrapper_BIND(GiWorldDraw)
		.def("geometry", &PyGiWorldDraw::geometry);
}

void bind_PyGiViewportDraw(pym& m) {
	auto cls = py::class_<PyGiViewportDraw, PyGiCommonDraw>(m, "PyGiViewportDraw");
	cls
		PyWrapper_BIND(GiViewportDraw)
		.def("geometry", &PyGiViewportDraw::geometry)
		.def("viewport", &PyGiViewportDraw::viewport)
		.def("sequenceNumber", &PyGiViewportDraw::sequenceNumber)
		.def("isValidId", &PyGiViewportDraw::isValidId, py::arg("acgi_id"))
		.def("viewportObjectId", &PyGiViewportDraw::viewportObjectId);
}

// ===========================================================================
// PyGiGeometry 族 —— 几何绘制接口（acgi.h 全量）
// ===========================================================================
void bind_PyGiGeometry(pym& m) {
	auto cls = py::class_<PyGiGeometry, PyRxObject>(m, "PyGiGeometry");
	cls
		PyWrapper_BIND(GiGeometry)
		// ---- 坐标变换 ----
		.def("getModelToWorldTransform", &PyGiGeometry::getModelToWorldTransform)
		.def("getWorldToModelTransform", &PyGiGeometry::getWorldToModelTransform)
		.def("pushModelTransformVector", [](PyGiGeometry& g, py::object n) { return g.pushModelTransformVector(AsVector3d(n)); }, py::arg("normal"))
		.def("pushModelTransform", [](PyGiGeometry& g, py::object m) { return g.pushModelTransform(AsMatrix3d(m)); }, py::arg("matrix"))
		.def("popModelTransform", &PyGiGeometry::popModelTransform)
		.def("pushPositionTransform", [](PyGiGeometry& g, int behavior, py::object offset) { return g.pushPositionTransform(behavior, offset); }, py::arg("behavior"), py::arg("offset"))
		.def("pushScaleTransform", [](PyGiGeometry& g, int behavior, py::object extents) { return g.pushScaleTransform(behavior, extents); }, py::arg("behavior"), py::arg("extents"))
		.def("pushOrientationTransform", &PyGiGeometry::pushOrientationTransform, py::arg("behavior"))
		// ---- 图元 ----
		.def("circle", [](const PyGiGeometry& g, py::object center, double radius, py::object normal) {
			return g.circle(AsPoint3d(center), radius, AsVector3d(normal));
		}, py::arg("center"), py::arg("radius"), py::arg("normal") = py::make_tuple(0.0, 0.0, 1.0))
		.def("circle3p", [](const PyGiGeometry& g, py::object p1, py::object p2, py::object p3) {
			return g.circle3p(AsPoint3d(p1), AsPoint3d(p2), AsPoint3d(p3));
		}, py::arg("p1"), py::arg("p2"), py::arg("p3"))
		.def("circularArc", [](const PyGiGeometry& g, py::object center, double radius, py::object normal,
			py::object startVector, double sweepAngle, int arcType) {
			return g.circularArc(AsPoint3d(center), radius, AsVector3d(normal), AsVector3d(startVector), sweepAngle, arcType);
		}, py::arg("center"), py::arg("radius"), py::arg("normal"), py::arg("start_vector"), py::arg("sweep_angle"),
			py::arg("arc_type") = static_cast<int>(kAcGiArcSimple))
		.def("circularArc3p", [](const PyGiGeometry& g, py::object start, py::object point, py::object end, int arcType) {
			return g.circularArc3p(AsPoint3d(start), AsPoint3d(point), AsPoint3d(end), arcType);
		}, py::arg("start"), py::arg("point"), py::arg("end"), py::arg("arc_type") = static_cast<int>(kAcGiArcSimple))
		.def("polyline", [](const PyGiGeometry& g, py::object pts, py::object normal, long long baseMarker) {
			return normal.is_none()
				? g.polyline(AsPoint3dList(pts, 2, "polyline"), std::nullopt, baseMarker)
				: g.polyline(AsPoint3dList(pts, 2, "polyline"), AsVector3d(normal), baseMarker);
		}, py::arg("points"), py::arg("normal") = py::none(), py::arg("base_subent_marker") = -1)
		.def("polylineObj", &PyGiGeometry::polylineObj, py::arg("polyline"))
		.def("polygon", [](const PyGiGeometry& g, py::object pts) { return g.polygon(AsPoint3dList(pts, 3, "polygon")); }, py::arg("points"))
		.def("mesh", [](const PyGiGeometry& g, unsigned int rows, unsigned int cols, py::object pts, bool autoNormals) {
			return g.mesh(rows, cols, AsPoint3dList(pts, 4, "mesh"), autoNormals);
		}, py::arg("rows"), py::arg("columns"), py::arg("points"), py::arg("auto_generate_normals") = true)
		.def("shell", [](const PyGiGeometry& g, py::object pts, py::object faceList, bool autoNormals) {
			return g.shell(AsPoint3dList(pts, 3, "shell"), py::cast<std::vector<int>>(faceList), autoNormals);
		}, py::arg("points"), py::arg("face_list"), py::arg("auto_generate_normals") = true)
		.def("text", [](const PyGiGeometry& g, py::object position, py::object normal, py::object direction,
			double height, double width, double oblique, const AcString& msg) {
			return g.text(AsPoint3d(position), AsVector3d(normal), AsVector3d(direction), height, width, oblique, msg);
		}, py::arg("position"), py::arg("normal"), py::arg("direction"),
			py::arg("height"), py::arg("width"), py::arg("oblique"), py::arg("msg"))
		.def("textStyled", [](const PyGiGeometry& g, py::object position, py::object normal, py::object direction,
			const AcString& msg, bool raw, const PyGiTextStyle& style) {
			return g.textStyled(AsPoint3d(position), AsVector3d(normal), AsVector3d(direction), msg, raw, style);
		}, py::arg("position"), py::arg("normal"), py::arg("direction"), py::arg("msg"),
			py::arg("raw") = false, py::arg("style"))
		.def("xline", [](const PyGiGeometry& g, py::object p1, py::object p2) { return g.xline(AsPoint3d(p1), AsPoint3d(p2)); }, py::arg("p1"), py::arg("p2"))
		.def("ray", [](const PyGiGeometry& g, py::object p1, py::object p2) { return g.ray(AsPoint3d(p1), AsPoint3d(p2)); }, py::arg("p1"), py::arg("p2"))
		.def("pline", &PyGiGeometry::pline, py::arg("polyline"), py::arg("from_index") = 0, py::arg("num_segs") = 0)
		.def("draw", &PyGiGeometry::draw, py::arg("drawable"))
		.def("image", [](const PyGiGeometry& g, const PyGiImageBGRA32& img, py::object position,
			py::object u, py::object v, int transparencyMode) {
			return g.image(img, AsPoint3d(position), AsVector3d(u), AsVector3d(v), transparencyMode);
		}, py::arg("image"), py::arg("position"), py::arg("u"), py::arg("v"),
			py::arg("transparency_mode") = static_cast<int>(AcGiGeometry::kTransparency8Bit))
		.def("rowOfDots", [](const PyGiGeometry& g, int count, py::object start, py::object step) {
			return g.rowOfDots(count, AsPoint3d(start), AsVector3d(step));
		}, py::arg("count"), py::arg("start"), py::arg("step"))
		.def("ellipticalArc", [](const PyGiGeometry& g, py::object center, py::object normal,
			double majorLen, double minorLen, double startRad, double endRad, double tiltRad, int arcType) {
			return g.ellipticalArc(AsPoint3d(center), AsVector3d(normal), majorLen, minorLen, startRad, endRad, tiltRad, arcType);
		}, py::arg("center"), py::arg("normal"), py::arg("major_axis_length"), py::arg("minor_axis_length"),
			py::arg("start_degree_rads"), py::arg("end_degree_rads"), py::arg("tilt_degree_rads"),
			py::arg("arc_type") = static_cast<int>(kAcGiArcSimple))
		.def("pushClipBoundary", [](PyGiGeometry& g, py::object pts2d, py::object normal, py::object origin,
			bool clipFront, double frontZ, bool clipBack, double backZ, bool drawBoundary) {
			std::vector<AcGePoint2d> pts;
			const py::sequence seq = py::cast<py::sequence>(pts2d);
			for (size_t i = 0; i < static_cast<size_t>(py::len(seq)); ++i)
				pts.push_back(AsPoint2d(seq[static_cast<py::ssize_t>(i)]));
			return g.pushClipBoundary(pts, AsVector3d(normal), AsPoint3d(origin), clipFront, frontZ, clipBack, backZ, drawBoundary);
		}, py::arg("points"), py::arg("normal") = py::make_tuple(0.0, 0.0, 1.0), py::arg("origin") = py::make_tuple(0.0, 0.0, 0.0),
			py::arg("clip_front") = false, py::arg("front_z") = 0.0,
			py::arg("clip_back") = false, py::arg("back_z") = 0.0, py::arg("draw_boundary") = false)
		.def("popClipBoundary", &PyGiGeometry::popClipBoundary)
		.def("worldLine", [](PyGiGeometry& g, py::object p1, py::object p2) { return g.worldLine(AsPoint3d(p1), AsPoint3d(p2)); }, py::arg("p1"), py::arg("p2"))
		.def("polypoint", [](const PyGiGeometry& g, py::object pts, py::object normal) {
			return normal.is_none()
				? g.polypoint(AsPoint3dList(pts, 1, "polypoint"), std::nullopt)
				: g.polypoint(AsPoint3dList(pts, 1, "polypoint"), AsVector3d(normal));
		}, py::arg("points"), py::arg("normal") = py::none())
		// ---- [ADD] mesh / shell 携带 AcGiEdgeData / FaceData / VertexData ----
		.def("mesh", [](const PyGiGeometry& g, unsigned int rows, unsigned int cols, py::object pts,
			py::object edgeData, py::object faceData, py::object vertexData, bool autoNormals) {
			return g.meshWithData(rows, cols, AsPoint3dList(pts, 4, "mesh"),
				edgeData.is_none() ? nullptr : &py::cast<GiEdgeDataHolder&>(edgeData),
				faceData.is_none() ? nullptr : &py::cast<GiFaceDataHolder&>(faceData),
				vertexData.is_none() ? nullptr : &py::cast<GiVertexDataHolder&>(vertexData),
				autoNormals);
		}, py::arg("rows"), py::arg("columns"), py::arg("points"),
			py::arg("edge_data") = py::none(), py::arg("face_data") = py::none(),
			py::arg("vertex_data") = py::none(), py::arg("auto_generate_normals") = true)
		.def("shell", [](const PyGiGeometry& g, py::object pts, py::object faceList,
			py::object edgeData, py::object faceData, py::object vertexData, bool autoNormals) {
			return g.shellWithData(AsPoint3dList(pts, 3, "shell"), py::cast<std::vector<int>>(faceList),
				edgeData.is_none() ? nullptr : &py::cast<GiEdgeDataHolder&>(edgeData),
				faceData.is_none() ? nullptr : &py::cast<GiFaceDataHolder&>(faceData),
				vertexData.is_none() ? nullptr : &py::cast<GiVertexDataHolder&>(vertexData),
				autoNormals);
		}, py::arg("points"), py::arg("face_list"),
			py::arg("edge_data") = py::none(), py::arg("face_data") = py::none(),
			py::arg("vertex_data") = py::none(), py::arg("auto_generate_normals") = true)
		// ---- [ADD] polyPolygon ----
		.def("polyPolygon", [](const PyGiGeometry& g, py::object numPolygonPositions, py::object polygonPositions,
			py::object numPolygonPoints, py::object polygonPoints,
			py::object outlineColors, py::object outlineTypes, py::object fillColors, py::object fillOpacities) {
			auto counts1 = py::cast<std::vector<unsigned int>>(numPolygonPositions);
			auto counts2 = py::cast<std::vector<unsigned int>>(numPolygonPoints);
			std::optional<std::vector<AcCmEntityColor>> oc, fc;
			std::optional<std::vector<int>> ot;
			std::optional<std::vector<AcCmTransparency>> fo;
			if (!outlineColors.is_none()) oc = py::cast<std::vector<AcCmEntityColor>>(outlineColors);
			if (!outlineTypes.is_none()) ot = py::cast<std::vector<int>>(outlineTypes);
			if (!fillColors.is_none()) fc = py::cast<std::vector<AcCmEntityColor>>(fillColors);
			if (!fillOpacities.is_none()) fo = py::cast<std::vector<AcCmTransparency>>(fillOpacities);
			return g.polyPolygon(counts1, AsPoint3dList(polygonPositions, 1, "polyPolygon.positions"),
				counts2, AsPoint3dList(polygonPoints, 1, "polyPolygon.points"), oc, ot, fc, fo);
		}, py::arg("num_polygon_positions"), py::arg("polygon_positions"),
			py::arg("num_polygon_points"), py::arg("polygon_points"),
			py::arg("outline_colors") = py::none(), py::arg("outline_types") = py::none(),
			py::arg("fill_colors") = py::none(), py::arg("fill_opacities") = py::none(),
			"polyPolygon(num_polygon_positions, polygon_positions, num_polygon_points, polygon_points, ...) -> bool")
		// ---- [ADD] ownerDraw(HDC) ----
		.def("ownerDraw", [](const PyGiGeometry& g, AcGiGdiDrawObjectPy& gdiObject, py::object position,
			py::object u, py::object v) {
			return g.ownerDraw(gdiObject, AsPoint3d(position), AsVector3d(u), AsVector3d(v));
		}, py::arg("gdi_object"), py::arg("position"), py::arg("u"), py::arg("v"));
}

void bind_PyGiWorldGeometry(pym& m) {
	auto cls = py::class_<PyGiWorldGeometry, PyGiGeometry>(m, "PyGiWorldGeometry");
	cls
		PyWrapper_BIND(GiWorldGeometry)
		.def("setExtents", [](const PyGiWorldGeometry& g, py::object minPt, py::object maxPt) {
			g.setExtents(AsPoint3d(minPt), AsPoint3d(maxPt));
		}, py::arg("min_point"), py::arg("max_point"))
		.def("startAttributesSegment", &PyGiWorldGeometry::startAttributesSegment);
}

void bind_PyGiViewportGeometry(pym& m) {
	auto cls = py::class_<PyGiViewportGeometry, PyGiGeometry>(m, "PyGiViewportGeometry");
	cls
		PyWrapper_BIND(GiViewportGeometry)
		.def("polylineEye", [](const PyGiViewportGeometry& g, py::object pts) { return g.polylineEye(AsPoint3dList(pts, 2, "polylineEye")); }, py::arg("points"))
		.def("polygonEye", [](const PyGiViewportGeometry& g, py::object pts) { return g.polygonEye(AsPoint3dList(pts, 3, "polygonEye")); }, py::arg("points"))
		.def("polylineDc", [](const PyGiViewportGeometry& g, py::object pts) { return g.polylineDc(AsPoint3dList(pts, 2, "polylineDc")); }, py::arg("points"))
		.def("polygonDc", [](const PyGiViewportGeometry& g, py::object pts) { return g.polygonDc(AsPoint3dList(pts, 3, "polygonDc")); }, py::arg("points"));
}

// ===========================================================================
// PyGiDrawable —— 可绘制对象（依赖上述类型均已注册）
// ===========================================================================
void bind_PyGiDrawable(pym& m) {
	auto cls = py::class_<PyGiDrawable, PyRxObject>(m, "PyGiDrawable");
	cls
		PyWrapper_BIND(GiDrawable)
		.def("isPersistent", &PyGiDrawable::isPersistent)
		.def("worldDraw", &PyGiDrawable::worldDraw, py::arg("wd"))
		.def("subWorldDraw", &PyGiDrawable::subWorldDraw, py::arg("wd"))
		.def("viewportDraw", &PyGiDrawable::viewportDraw, py::arg("vpd"))
		.def("subViewportDraw", &PyGiDrawable::subViewportDraw, py::arg("vd"))
		.def("viewportDrawLogicalFlags", &PyGiDrawable::viewportDrawLogicalFlags, py::arg("vd"))
		.def("subViewportDrawLogicalFlags", &PyGiDrawable::subViewportDrawLogicalFlags, py::arg("svd"))
		.def("setAttributes", &PyGiDrawable::setAttributes, py::arg("traits"))
		.def("subSetAttributes", &PyGiDrawable::subSetAttributes, py::arg("traits"))
		.def("drawableType", [](const PyGiDrawable& d) { return static_cast<int>(d.drawableType()); })
		.def("rolloverHit", [](PyGiDrawable& d, int nSubentId, int nMouseFlags, bool bReset) {
			return d.rolloverHit(static_cast<Adesk::ULongPtr>(nSubentId), static_cast<Adesk::ULongPtr>(nMouseFlags), bReset ? Adesk::kTrue : Adesk::kFalse);
		}, py::arg("nSubentId"), py::arg("nMouseFlags"), py::arg("bReset"));
}

// ===========================================================================
// bind_PyGi: PyArx.Gi 子模块注册函数（由 PyArxBinder.cpp 调用）；依赖 PyArx.Db
// （PyRxObject/ObjectId/PyDbEntity 均注册于 PyArx.Db），
// 绑定顺序：枚举 → AcCm/AcGi 值类型 → AcGiFill 族 → Polyline/Image/Viewport →
//           Traits → Draw 上下文 → Geometry 族 → Drawable → 文字 → 瞬态。
// ===========================================================================

void bind_PyGiTextStyle(pym& m) {
	auto cls = py::class_<PyGiTextStyle>(m, "PyGiTextStyle");
	cls.def(py::init<const std::wstring&, const std::wstring&, double, double, double,
		double, bool, bool, bool, bool, bool, bool>(),
		py::arg("font_file") = std::wstring(),
		py::arg("big_font") = std::wstring(),
		py::arg("text_size") = 1.0,
		py::arg("x_scale") = 1.0,
		py::arg("oblique_angle") = 0.0,
		py::arg("tracking_percent") = 1.0,
		py::arg("backward") = false,
		py::arg("upside_down") = false,
		py::arg("vertical") = false,
		py::arg("overlined") = false,
		py::arg("underlined") = false,
		py::arg("strikethrough") = false)
		.def("valid", &PyGiTextStyle::valid)
		.def("setFileName", [](PyGiTextStyle& s, const std::wstring& v) { s.ptr()->setFileName(v.c_str()); }, py::arg("name"))
		.def("fileName", [](const PyGiTextStyle& s) { const ACHAR* p = s.ptr()->fileName(); return p ? std::wstring(p) : std::wstring(); })
		.def("setBigFontFileName", [](PyGiTextStyle& s, const std::wstring& v) { s.ptr()->setBigFontFileName(v.c_str()); }, py::arg("name"))
		.def("bigFontFileName", [](const PyGiTextStyle& s) { const ACHAR* p = s.ptr()->bigFontFileName(); return p ? std::wstring(p) : std::wstring(); })
		.def("setTextSize", [](PyGiTextStyle& s, double v) { s.ptr()->setTextSize(v); }, py::arg("size"))
		.def("textSize", [](const PyGiTextStyle& s) { return s.ptr()->textSize(); })
		.def("setXScale", [](PyGiTextStyle& s, double v) { s.ptr()->setXScale(v); }, py::arg("xScale"))
		.def("xScale", [](const PyGiTextStyle& s) { return s.ptr()->xScale(); })
		.def("setObliquingAngle", [](PyGiTextStyle& s, double v) { s.ptr()->setObliquingAngle(v); }, py::arg("angle"))
		.def("obliquingAngle", [](const PyGiTextStyle& s) { return s.ptr()->obliquingAngle(); })
		.def("setTrackingPercent", [](PyGiTextStyle& s, double v) { s.ptr()->setTrackingPercent(v); }, py::arg("percent"))
		.def("trackingPercent", [](const PyGiTextStyle& s) { return s.ptr()->trackingPercent(); })
		.def("setBackward", [](PyGiTextStyle& s, bool v) { s.ptr()->setBackward(v ? Adesk::kTrue : Adesk::kFalse); }, py::arg("on"))
		.def("isBackward", [](const PyGiTextStyle& s) { return s.ptr()->isBackward() != 0; })
		.def("setUpsideDown", [](PyGiTextStyle& s, bool v) { s.ptr()->setUpsideDown(v ? Adesk::kTrue : Adesk::kFalse); }, py::arg("on"))
		.def("isUpsideDown", [](const PyGiTextStyle& s) { return s.ptr()->isUpsideDown() != 0; })
		.def("setVertical", [](PyGiTextStyle& s, bool v) { s.ptr()->setVertical(v ? Adesk::kTrue : Adesk::kFalse); }, py::arg("on"))
		.def("isVertical", [](const PyGiTextStyle& s) { return s.ptr()->isVertical() != 0; })
		.def("setUnderlined", [](PyGiTextStyle& s, bool v) { s.ptr()->setUnderlined(v ? Adesk::kTrue : Adesk::kFalse); }, py::arg("on"))
		.def("isUnderlined", [](const PyGiTextStyle& s) { return s.ptr()->isUnderlined() != 0; })
		.def("setOverlined", [](PyGiTextStyle& s, bool v) { s.ptr()->setOverlined(v ? Adesk::kTrue : Adesk::kFalse); }, py::arg("on"))
		.def("isOverlined", [](const PyGiTextStyle& s) { return s.ptr()->isOverlined() != 0; })
		.def("setStrikethrough", [](PyGiTextStyle& s, bool v) { s.ptr()->setStrikethrough(v ? Adesk::kTrue : Adesk::kFalse); }, py::arg("on"))
		.def("isStrikethrough", [](const PyGiTextStyle& s) { return s.ptr()->isStrikethrough() != 0; })
		.def("styleName", [](const PyGiTextStyle& s) { const ACHAR* p = s.ptr()->styleName(); return p ? std::wstring(p) : std::wstring(); })
		.def("setStyleName", [](PyGiTextStyle& s, const std::wstring& v) { return static_cast<int>(s.ptr()->setStyleName(v.c_str())); }, py::arg("name"))
		.def("extents", [](const PyGiTextStyle& s, const std::wstring& text, bool penup, bool raw) {
			AcGePoint2d e = s.ptr()->extents(text.c_str(), penup ? Adesk::kTrue : Adesk::kFalse,
				static_cast<int>(text.size()),
				raw ? Adesk::kTrue : Adesk::kFalse);
			return py::make_tuple(e.x, e.y);
		}, py::arg("text"), py::arg("penup") = false, py::arg("raw") = false)
		.def("extentsBox", [](const PyGiTextStyle& s, const std::wstring& text, bool penup, bool raw) {
			AcGePoint2d mn(0.0, 0.0), mx(0.0, 0.0);
			s.ptr()->extentsBox(text.c_str(), penup ? Adesk::kTrue : Adesk::kFalse,
				static_cast<int>(text.size()),
				raw ? Adesk::kTrue : Adesk::kFalse, mn, mx);
			return py::make_tuple(py::make_tuple(mn.x, mn.y), py::make_tuple(mx.x, mx.y));
		}, py::arg("text"), py::arg("penup") = false, py::arg("raw") = false);
}

void bind_PyGiTextEngine(pym& m) {
	auto cls = py::class_<PyGiTextEngine>(m, "PyGiTextEngine");
	cls.def(py::init<>())
		.def_static("available", &PyGiTextEngine::available)
		.def("valid", &PyGiTextEngine::valid)
		.def("tessellate", &PyGiTextEngine::tessellate,
			py::arg("style"), py::arg("text"),
			py::arg("raw") = false, py::arg("deviation") = 0.05)
		.def("getExtents", &PyGiTextEngine::getExtents,
			py::arg("style"), py::arg("text"), py::arg("raw") = false);
}

void bind_PyGi(pym& m) {
	m.doc() = "AutoCAD ARX AcGi Graphics Interface";
	py::module_::import("PyArxCore.Db"); // PyRxObject / ObjectId / PyDbEntity type_info（PyArxCore.Db 子模块）

	bind_PyGiEnums(m);
	// 值类型（AcGi 方法签名依赖，必须先注册）
	bind_PyCmEntityColor(m);
	bind_PyCmTransparency(m);
	bind_PyGiVariant(m);
	bind_PyGiMapper(m);
	bind_PyGiFill(m);
	bind_PyGiPolyline(m);
	bind_PyGiImage(m);
	bind_PyGiViewport(m);

	// 第二轮新增：mesh/shell 属性容器、绘制上下文与 GDI 自绘、图形流与视觉样式、
	// 材质/灯光/渲染环境
	bind_PyGiMeshData(m);
	bind_PyGiContextGdi(m);
	bind_PyGiStreamStyle(m);
	bind_PyGiMaterial(m);
	bind_PyGiLight(m);
	bind_PyGiEnvironment(m);

	// 特性 / 绘制上下文
	bind_PyGiSubEntityTraits(m);
	bind_PyGiDrawableTraits(m);
	bind_PyGiCommonDraw(m);
	bind_PyGiWorldDraw(m);
	bind_PyGiViewportDraw(m);

	// 几何族
	bind_PyGiGeometry(m);
	bind_PyGiWorldGeometry(m);
	bind_PyGiViewportGeometry(m);

	bind_PyGiDrawable(m);

	// 文字
	bind_PyGiTextStyle(m);
	bind_PyGiTextEngine(m);

	// 瞬态图形 + Python 自绘桥
	bind_PyGiTransient(m);
}
