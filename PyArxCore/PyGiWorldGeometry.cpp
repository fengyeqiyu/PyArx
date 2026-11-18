#include "stdafx.h"
#include "PyRxObject.h"
#include "PyRxClass.h"
#include "PyGiWorldGeometry.h"
#include "PyGiSubEntityTraits.h"
#include "PyGiPolyline.h"
#include "PyGiImage.h"
#include "PyGiTextStyle.h"
#include "PyGiViewport.h"
#include "PyGiDrawable.h"
#include "PyDbPolyline.h"
#include "PyGiMeshData.h"
#include "PyGiContextGdi.h"

PyWrapper_Cpp(GiCommonDraw);
PyWrapper_Cpp(GiWorldDraw);
PyWrapper_Cpp(GiViewportDraw);
PyWrapper_Cpp(GiGeometry);
PyWrapper_Cpp(GiWorldGeometry);
PyWrapper_Cpp(GiViewportGeometry);

namespace {

bool Is2dPoint(const py::handle& h) {
	if (py::isinstance<AcGePoint2d>(h)) return true;
	if (py::isinstance<AcGePoint3d>(h)) return false;
	if (!py::isinstance<py::sequence>(h))
		throw py::type_error("需要点坐标（AcGePoint2d/AcGePoint3d/(x,y[,z])）");
	return py::len(h) == 2;
}

} // namespace

// ===========================================================================
// PyGiCommonDraw
// ===========================================================================
PyGiSubEntityTraits PyGiCommonDraw::subEntityTraits() const {
	return PyGiSubEntityTraits(&impObj()->subEntityTraits(), false);
}
AcGiRegenType PyGiCommonDraw::regenType() const {
	return impObj()->regenType();
}
bool PyGiCommonDraw::regenAbort() const {
	if (!impObj()) return false;
	return impObj()->regenAbort() == Adesk::kTrue;
}
bool PyGiCommonDraw::isDragging() const {
	if (!impObj()) return false;
	return impObj()->isDragging() == Adesk::kTrue;
}
double PyGiCommonDraw::deviation(int deviationType, const AcGePoint3d& pt) const {
	if (!impObj()) return 0.0;
	return impObj()->deviation(static_cast<AcGiDeviationType>(deviationType), pt);
}
unsigned int PyGiCommonDraw::numberOfIsolines() const {
	if (!impObj()) return 0;
	return static_cast<unsigned int>(impObj()->numberOfIsolines());
}
bool PyGiCommonDraw::secondaryCall() const {
	if (!impObj()) return false;
	return impObj()->secondaryCall();
}
GiContextRef* PyGiCommonDraw::context() {
	return new GiContextRef(impObj()->context());
}

PyGiWorldGeometry PyGiWorldDraw::geometry() const {
	return PyGiWorldGeometry(&impObj()->geometry());
}
PyGiViewportGeometry PyGiViewportDraw::geometry() const {
	return PyGiViewportGeometry(&impObj()->geometry());
}
PyGiViewport PyGiViewportDraw::viewport() const {
	return PyGiViewport(&impObj()->viewport());
}
unsigned int PyGiViewportDraw::sequenceNumber() const {
	if (!impObj()) return 0;
	return static_cast<unsigned int>(impObj()->sequenceNumber());
}
bool PyGiViewportDraw::isValidId(unsigned long long acgiId) const {
	if (!impObj()) return false;
	return impObj()->isValidId(static_cast<Adesk::ULongPtr>(acgiId)) == Adesk::kTrue;
}
AcDbObjectId PyGiViewportDraw::viewportObjectId() const {
	return impObj()->viewportObjectId();
}

// ===========================================================================
// PyGiGeometry —— 坐标变换
// ===========================================================================
AcGeMatrix3d PyGiGeometry::getModelToWorldTransform() const {
	AcGeMatrix3d m = AcGeMatrix3d::kIdentity;
	impGeom()->getModelToWorldTransform(m);
	return m;
}

AcGeMatrix3d PyGiGeometry::getWorldToModelTransform() const {
	AcGeMatrix3d m = AcGeMatrix3d::kIdentity;
	impGeom()->getWorldToModelTransform(m);
	return m;
}

bool PyGiGeometry::pushModelTransformVector(const AcGeVector3d& normal) {
	return impGeom()->pushModelTransform(normal) == Adesk::kTrue;
}

bool PyGiGeometry::pushModelTransform(const AcGeMatrix3d& mat) {
	return impGeom()->pushModelTransform(mat) == Adesk::kTrue;
}

bool PyGiGeometry::popModelTransform() {
	return impGeom()->popModelTransform() == Adesk::kTrue;
}

AcGeMatrix3d PyGiGeometry::pushPositionTransform(int behavior, py::object offset) {
	const AcGiPositionTransformBehavior b = static_cast<AcGiPositionTransformBehavior>(behavior);
	if (Is2dPoint(offset))
		return impGeom()->pushPositionTransform(b, AsPoint2d(offset));
	return impGeom()->pushPositionTransform(b, AsPoint3d(offset));
}

AcGeMatrix3d PyGiGeometry::pushScaleTransform(int behavior, py::object extents) {
	const AcGiScaleTransformBehavior b = static_cast<AcGiScaleTransformBehavior>(behavior);
	if (Is2dPoint(extents))
		return impGeom()->pushScaleTransform(b, AsPoint2d(extents));
	return impGeom()->pushScaleTransform(b, AsPoint3d(extents));
}

AcGeMatrix3d PyGiGeometry::pushOrientationTransform(int behavior) {
	return impGeom()->pushOrientationTransform(static_cast<AcGiOrientationTransformBehavior>(behavior));
}

// ===========================================================================
// PyGiGeometry —— 图元
// ===========================================================================
bool PyGiGeometry::circle(const AcGePoint3d& center, double radius, const AcGeVector3d& normal) const {
	return impGeom()->circle(center, radius, normal) == Adesk::kTrue;
}

bool PyGiGeometry::circle3p(const AcGePoint3d& p1, const AcGePoint3d& p2, const AcGePoint3d& p3) const {
	return impGeom()->circle(p1, p2, p3) == Adesk::kTrue;
}

bool PyGiGeometry::circularArc(const AcGePoint3d& center, double radius, const AcGeVector3d& normal,
	const AcGeVector3d& startVector, double sweepAngle, int arcType) const {
	return impGeom()->circularArc(center, radius, normal, startVector, sweepAngle,
		static_cast<AcGiArcType>(arcType)) == Adesk::kTrue;
}

bool PyGiGeometry::circularArc3p(const AcGePoint3d& start, const AcGePoint3d& point, const AcGePoint3d& end, int arcType) const {
	return impGeom()->circularArc(start, point, end, static_cast<AcGiArcType>(arcType)) == Adesk::kTrue;
}

bool PyGiGeometry::polyline(const std::vector<AcGePoint3d>& pts, const std::optional<AcGeVector3d>& normal,
	long long baseSubEntMarker) const {
	if (pts.empty()) return false;
	std::vector<AcGePoint3d> verts = pts;
	AcGeVector3d nrm = normal.value_or(AcGeVector3d::kZAxis);
	return impGeom()->polyline(static_cast<Adesk::UInt32>(verts.size()), verts.data(),
		normal ? &nrm : nullptr, static_cast<Adesk::LongPtr>(baseSubEntMarker)) == Adesk::kTrue;
}

bool PyGiGeometry::polylineObj(const PyGiPolyline& polylineObj) const {
	return impGeom()->polyline(*polylineObj.impObj()) == Adesk::kTrue;
}

bool PyGiGeometry::polygon(const std::vector<AcGePoint3d>& pts) const {
	if (pts.empty()) return false;
	std::vector<AcGePoint3d> verts = pts;
	return impGeom()->polygon(static_cast<Adesk::UInt32>(verts.size()), verts.data()) == Adesk::kTrue;
}

bool PyGiGeometry::mesh(unsigned int rows, unsigned int cols, const std::vector<AcGePoint3d>& pts, bool autoGenerateNormals) const {
	if (rows * cols != pts.size() || pts.empty())
		throw py::value_error("mesh 点数必须等于 rows * columns");
	std::vector<AcGePoint3d> verts = pts;
	// AcGiEdgeData / AcGiFaceData / AcGiVertexData 为裸指针数组接口，Python 侧不导出，传 NULL
	return impGeom()->mesh(rows, cols, verts.data(), nullptr, nullptr, nullptr, autoGenerateNormals) == Adesk::kTrue;
}

bool PyGiGeometry::shell(const std::vector<AcGePoint3d>& pts, const std::vector<int>& faceList, bool autoGenerateNormals) const {
	if (pts.empty()) return false;
	std::vector<AcGePoint3d> verts = pts;
	std::vector<Adesk::Int32> faces;
	faces.reserve(faceList.size());
	for (int v : faceList) faces.push_back(static_cast<Adesk::Int32>(v));
	return impGeom()->shell(static_cast<Adesk::UInt32>(verts.size()), verts.data(),
		static_cast<Adesk::UInt32>(faces.size()), faces.empty() ? nullptr : faces.data(),
		nullptr, nullptr, nullptr, nullptr, autoGenerateNormals) == Adesk::kTrue;
}

bool PyGiGeometry::meshWithData(unsigned int rows, unsigned int cols, const std::vector<AcGePoint3d>& pts,
	const GiEdgeDataHolder* edgeData, const GiFaceDataHolder* faceData,
	const GiVertexDataHolder* vertexData, bool autoGenerateNormals) const {
	if (rows * cols != pts.size() || pts.empty())
		throw py::value_error("mesh 点数必须等于 rows * columns");
	std::vector<AcGePoint3d> verts = pts;
	return impGeom()->mesh(rows, cols, verts.data(),
		edgeData ? &edgeData->data() : nullptr,
		faceData ? &faceData->data() : nullptr,
		vertexData ? &vertexData->data() : nullptr,
		autoGenerateNormals) == Adesk::kTrue;
}

bool PyGiGeometry::shellWithData(const std::vector<AcGePoint3d>& pts, const std::vector<int>& faceList,
	const GiEdgeDataHolder* edgeData, const GiFaceDataHolder* faceData,
	const GiVertexDataHolder* vertexData, bool autoGenerateNormals) const {
	if (pts.empty()) return false;
	std::vector<AcGePoint3d> verts = pts;
	std::vector<Adesk::Int32> faces;
	faces.reserve(faceList.size());
	for (int v : faceList) faces.push_back(static_cast<Adesk::Int32>(v));
	return impGeom()->shell(static_cast<Adesk::UInt32>(verts.size()), verts.data(),
		static_cast<Adesk::UInt32>(faces.size()), faces.empty() ? nullptr : faces.data(),
		edgeData ? &edgeData->data() : nullptr,
		faceData ? &faceData->data() : nullptr,
		vertexData ? &vertexData->data() : nullptr,
		nullptr, autoGenerateNormals) == Adesk::kTrue;
}

bool PyGiGeometry::polyPolygon(const std::vector<unsigned int>& numPolygonPositions,
	const std::vector<AcGePoint3d>& polygonPositions,
	const std::vector<unsigned int>& numPolygonPoints,
	const std::vector<AcGePoint3d>& polygonPoints,
	const std::optional<std::vector<AcCmEntityColor>>& outlineColors,
	const std::optional<std::vector<int>>& outlineTypes,
	const std::optional<std::vector<AcCmEntityColor>>& fillColors,
	const std::optional<std::vector<AcCmTransparency>>& fillOpacities) const {
	const size_t numPolygons = numPolygonPositions.size();
	if (numPolygons == 0) return false;
	if (numPolygonPoints.size() < numPolygons)
		throw py::value_error("numPolygonPoints 的长度必须与 numPolygonPositions 相同");
	AcGePoint3d dummy(0.0, 0.0, 0.0);

	std::vector<AcGiLineType> types;
	if (outlineTypes) {
		types.reserve(outlineTypes->size());
		for (int t : *outlineTypes) types.push_back(static_cast<AcGiLineType>(t));
	}

	return impGeom()->polyPolygon(static_cast<Adesk::UInt32>(numPolygons),
		numPolygonPositions.data(),
		polygonPositions.empty() ? &dummy : polygonPositions.data(),
		numPolygonPoints.data(),
		polygonPoints.empty() ? &dummy : polygonPoints.data(),
		(outlineColors && !outlineColors->empty()) ? outlineColors->data() : nullptr,
		types.empty() ? nullptr : types.data(),
		(fillColors && !fillColors->empty()) ? fillColors->data() : nullptr,
		(fillOpacities && !fillOpacities->empty()) ? fillOpacities->data() : nullptr) == Adesk::kTrue;
}

bool PyGiGeometry::ownerDraw(AcGiGdiDrawObjectPy& gdiObject, const AcGePoint3d& position,
	const AcGeVector3d& u, const AcGeVector3d& v) const {
	return impGeom()->ownerDraw(&gdiObject, position, u, v) == Adesk::kTrue;
}

bool PyGiGeometry::text(const AcGePoint3d& position, const AcGeVector3d& normal, const AcGeVector3d& direction,
	double height, double width, double oblique, const AcString& msg) const {
	return impGeom()->text(position, normal, direction, height, width, oblique, msg.constPtr()) == Adesk::kTrue;
}

bool PyGiGeometry::textStyled(const AcGePoint3d& position, const AcGeVector3d& normal, const AcGeVector3d& direction,
	const AcString& msg, bool raw, const PyGiTextStyle& style) const {
	if (!style.valid()) throw py::value_error("AcGiTextStyle 无效");
	return impGeom()->text(position, normal, direction, msg.constPtr(),
		static_cast<Adesk::Int32>(msg.length()), raw ? Adesk::kTrue : Adesk::kFalse, *style.ptr()) == Adesk::kTrue;
}

bool PyGiGeometry::xline(const AcGePoint3d& p1, const AcGePoint3d& p2) const {
	return impGeom()->xline(p1, p2) == Adesk::kTrue;
}

bool PyGiGeometry::ray(const AcGePoint3d& p1, const AcGePoint3d& p2) const {
	return impGeom()->ray(p1, p2) == Adesk::kTrue;
}

bool PyGiGeometry::pline(const PyDbPolyline& lwBuf, unsigned int fromIndex, unsigned int numSegs) const {
	return impGeom()->pline(*lwBuf.impObj(), fromIndex, numSegs) == Adesk::kTrue;
}

bool PyGiGeometry::draw(PyGiDrawable& drawable) const {
	AcRxObject* obj = drawable.impObj();
	if (!obj) return false;
	return impGeom()->draw(static_cast<AcGiDrawable*>(obj)) == Adesk::kTrue;
}

bool PyGiGeometry::image(const PyGiImageBGRA32& img, const AcGePoint3d& position,
	const AcGeVector3d& u, const AcGeVector3d& v, int transparencyMode) const {
	return impGeom()->image(img.impObj(), position, u, v,
		static_cast<AcGiGeometry::TransparencyMode>(transparencyMode)) == Adesk::kTrue;
}

bool PyGiGeometry::rowOfDots(int count, const AcGePoint3d& start, const AcGeVector3d& step) const {
	return impGeom()->rowOfDots(count, start, step) == Adesk::kTrue;
}

bool PyGiGeometry::ellipticalArc(const AcGePoint3d& center, const AcGeVector3d& normal,
	double majorAxisLength, double minorAxisLength,
	double startDegreeInRads, double endDegreeInRads, double tiltDegreeInRads, int arcType) const {
	return impGeom()->ellipticalArc(center, normal, majorAxisLength, minorAxisLength,
		startDegreeInRads, endDegreeInRads, tiltDegreeInRads,
		static_cast<AcGiArcType>(arcType)) == Adesk::kTrue;
}

bool PyGiGeometry::pushClipBoundary(const std::vector<AcGePoint2d>& pts2d, const AcGeVector3d& normal,
	const AcGePoint3d& origin, bool clipFront, double frontZ, bool clipBack, double backZ,
	bool drawBoundary) {
	if (pts2d.size() < 3) throw py::value_error("裁剪边界至少需要 3 个二维点");

	AcGiClipBoundary boundary;
	boundary.m_vNormal = normal;
	boundary.m_ptPoint = origin;
	boundary.m_aptPoints.setLogicalLength(static_cast<int>(pts2d.size()));
	for (size_t i = 0; i < pts2d.size(); ++i)
		boundary.m_aptPoints[static_cast<int>(i)] = pts2d[i];

	// 模型 -> 裁剪空间：先按法向转到平面坐标系，再把 origin 平移成原点
	AcGeMatrix3d toClip = AcGeMatrix3d::worldToPlane(normal);
	const AcGePoint3d originInClip = toClip * origin;
	toClip.setTranslation(AcGeVector3d(-originInClip.x, -originInClip.y, -originInClip.z));
	boundary.m_xToClipSpace = toClip;

	boundary.m_bClippingFront = clipFront ? Adesk::kTrue : Adesk::kFalse;
	boundary.m_bClippingBack = clipBack ? Adesk::kTrue : Adesk::kFalse;
	boundary.m_dFrontClipZ = frontZ;
	boundary.m_dBackClipZ = backZ;
	boundary.m_bDrawBoundary = drawBoundary ? Adesk::kTrue : Adesk::kFalse;

	return impGeom()->pushClipBoundary(&boundary) == Adesk::kTrue;
}

void PyGiGeometry::popClipBoundary() {
	impGeom()->popClipBoundary();
}

bool PyGiGeometry::worldLine(const AcGePoint3d& p1, const AcGePoint3d& p2) {
	AcGePoint3d pts[2] = { p1, p2 };
	return impGeom()->worldLine(pts) == Adesk::kTrue;
}

bool PyGiGeometry::polypoint(const std::vector<AcGePoint3d>& pts, const std::optional<AcGeVector3d>& normal) const {
	if (pts.empty()) return false;
	std::vector<AcGePoint3d> verts = pts;
	std::vector<AcGeVector3d> normals;
	if (normal) normals.assign(verts.size(), *normal);
	return impGeom()->polypoint(static_cast<Adesk::UInt32>(verts.size()), verts.data(),
		normal ? normals.data() : nullptr, nullptr) == Adesk::kTrue;
}

// ===========================================================================
// PyGiWorldGeometry / PyGiViewportGeometry
// ===========================================================================
void PyGiWorldGeometry::setExtents(const AcGePoint3d& minPt, const AcGePoint3d& maxPt) const {
	AcGePoint3d extents[2] = { minPt, maxPt };
	impWorld()->setExtents(extents);
}

void PyGiWorldGeometry::startAttributesSegment() {
	impWorld()->startAttributesSegment();
}

bool PyGiViewportGeometry::polylineEye(const std::vector<AcGePoint3d>& pts) const {
	if (pts.empty()) return false;
	std::vector<AcGePoint3d> verts = pts;
	return impViewportGeom()->polylineEye(static_cast<Adesk::UInt32>(verts.size()), verts.data()) == Adesk::kTrue;
}

bool PyGiViewportGeometry::polygonEye(const std::vector<AcGePoint3d>& pts) const {
	if (pts.empty()) return false;
	std::vector<AcGePoint3d> verts = pts;
	return impViewportGeom()->polygonEye(static_cast<Adesk::UInt32>(verts.size()), verts.data()) == Adesk::kTrue;
}

bool PyGiViewportGeometry::polylineDc(const std::vector<AcGePoint3d>& pts) const {
	if (pts.empty()) return false;
	std::vector<AcGePoint3d> verts = pts;
	return impViewportGeom()->polylineDc(static_cast<Adesk::UInt32>(verts.size()), verts.data()) == Adesk::kTrue;
}

bool PyGiViewportGeometry::polygonDc(const std::vector<AcGePoint3d>& pts) const {
	if (pts.empty()) return false;
	std::vector<AcGePoint3d> verts = pts;
	return impViewportGeom()->polygonDc(static_cast<Adesk::UInt32>(verts.size()), verts.data()) == Adesk::kTrue;
}
