#pragma once
#include "PyRxObject.h"
#include "PyGiTypes.h"

#pragma pack (push, 8)
class PyGiDrawable;
class PyGiWorldGeometry;
class PyGiViewportGeometry;
class PyDbPolyline;
class PyGiSubEntityTraits;
class PyGiPolyline;
class PyGiViewport;
class PyGiImageBGRA32;
class PyGiTextStyle;
class GiContextRef;
class AcGiGdiDrawObjectPy;
class GiEdgeDataHolder;
class GiFaceDataHolder;
class GiVertexDataHolder;


class PyGiCommonDraw : public PyRxObject
{
	PyWrapper_H(GiCommonDraw)
public:
	PyGiCommonDraw(AcGiCommonDraw* ptr, bool autoDelete = false) :PyRxObject(ptr, autoDelete, false) {}
	virtual ~PyGiCommonDraw() override = default;
	PyGiSubEntityTraits subEntityTraits() const;
	AcGiRegenType regenType() const;
	Adesk::Boolean regenAbort() const;
	Adesk::Boolean isDragging() const;
	// [ADD] SDK: deviation(AcGiDeviationType, const AcGePoint3d&) / numberOfIsolines / secondaryCall
	double deviation(int deviationType, const AcGePoint3d& pt) const;
	unsigned int numberOfIsolines() const;
	bool secondaryCall() const;
	// [ADD] SDK: AcGiContext* context()（acgi.h:861）
	GiContextRef* context();
};


class PyGiWorldDraw : public PyGiCommonDraw
{
	PyWrapper_H(GiWorldDraw)
public:
	PyGiWorldDraw(AcGiWorldDraw* ptr) :PyGiCommonDraw(ptr) {
	}
	virtual ~PyGiWorldDraw() override = default;
	PyGiWorldGeometry geometry() const;
};



class PyGiViewportDraw : public PyGiCommonDraw
{
	PyWrapper_H(GiViewportDraw)
public:
	PyGiViewportDraw(AcGiViewportDraw* ptr, bool autoDelete = false) :PyGiCommonDraw(ptr) {
	}
	virtual ~PyGiViewportDraw() override = default;

	PyGiViewportGeometry geometry() const;
	// [ADD] SDK: viewport() / sequenceNumber() / isValidId() / viewportObjectId()
	PyGiViewport viewport() const;
	unsigned int sequenceNumber() const;
	bool isValidId(unsigned long long acgiId) const;
	AcDbObjectId viewportObjectId() const;

};

//-----------------------------------------------------------------------------------------

// AcGiGeometry：世界/视口几何绘制的公共基类。SDK 里所有图元都从这里走，
// 原封装为空类（只有几何绘制入口 geometry()），此处按 acgi.h 全量补齐。
class PyGiGeometry : public PyRxObject
{
	PyWrapper_H(GiGeometry)
public:
	PyGiGeometry(AcGiGeometry* ptr) :PyRxObject(ptr, false, false) {
	};
	virtual ~PyGiGeometry() override = default;

	AcGiGeometry* impGeom() const { return static_cast<AcGiGeometry*>(impObj()); }

	// ---- 坐标变换 ----
	AcGeMatrix3d getModelToWorldTransform() const;
	AcGeMatrix3d getWorldToModelTransform() const;
	bool pushModelTransformVector(const AcGeVector3d& normal);
	bool pushModelTransform(const AcGeMatrix3d& mat);
	bool popModelTransform();
	AcGeMatrix3d pushPositionTransform(int behavior, py::object offset);
	AcGeMatrix3d pushScaleTransform(int behavior, py::object extents);
	AcGeMatrix3d pushOrientationTransform(int behavior);

	// ---- 图元 ----
	bool circle(const AcGePoint3d& center, double radius, const AcGeVector3d& normal) const;
	bool circle3p(const AcGePoint3d& p1, const AcGePoint3d& p2, const AcGePoint3d& p3) const;
	bool circularArc(const AcGePoint3d& center, double radius, const AcGeVector3d& normal,
		const AcGeVector3d& startVector, double sweepAngle, int arcType) const;
	bool circularArc3p(const AcGePoint3d& start, const AcGePoint3d& point, const AcGePoint3d& end, int arcType) const;
	bool polyline(const std::vector<AcGePoint3d>& pts, const std::optional<AcGeVector3d>& normal,
		long long baseSubEntMarker) const;
	bool polylineObj(const PyGiPolyline& polylineObj) const;
	bool polygon(const std::vector<AcGePoint3d>& pts) const;
	bool mesh(unsigned int rows, unsigned int cols, const std::vector<AcGePoint3d>& pts, bool autoGenerateNormals) const;
	bool shell(const std::vector<AcGePoint3d>& pts, const std::vector<int>& faceList, bool autoGenerateNormals) const;
	// [ADD] SDK: mesh / shell 携带逐边、逐面、逐顶点属性（acgi.h:1761 / 1770）
	bool meshWithData(unsigned int rows, unsigned int cols, const std::vector<AcGePoint3d>& pts,
		const GiEdgeDataHolder* edgeData, const GiFaceDataHolder* faceData,
		const GiVertexDataHolder* vertexData, bool autoGenerateNormals) const;
	bool shellWithData(const std::vector<AcGePoint3d>& pts, const std::vector<int>& faceList,
		const GiEdgeDataHolder* edgeData, const GiFaceDataHolder* faceData,
		const GiVertexDataHolder* vertexData, bool autoGenerateNormals) const;
	// [ADD] SDK: polyPolygon（acgi.h:1751）
	bool polyPolygon(const std::vector<unsigned int>& numPolygonPositions,
		const std::vector<AcGePoint3d>& polygonPositions,
		const std::vector<unsigned int>& numPolygonPoints,
		const std::vector<AcGePoint3d>& polygonPoints,
		const std::optional<std::vector<AcCmEntityColor>>& outlineColors,
		const std::optional<std::vector<int>>& outlineTypes,
		const std::optional<std::vector<AcCmEntityColor>>& fillColors,
		const std::optional<std::vector<AcCmTransparency>>& fillOpacities) const;
	// [ADD] SDK: ownerDraw(HDC)（acgi.h:1861）
	bool ownerDraw(AcGiGdiDrawObjectPy& gdiObject, const AcGePoint3d& position,
		const AcGeVector3d& u, const AcGeVector3d& v) const;
	bool text(const AcGePoint3d& position, const AcGeVector3d& normal, const AcGeVector3d& direction,
		double height, double width, double oblique, const AcString& msg) const;
	bool textStyled(const AcGePoint3d& position, const AcGeVector3d& normal, const AcGeVector3d& direction,
		const AcString& msg, bool raw, const PyGiTextStyle& style) const;
	bool xline(const AcGePoint3d& p1, const AcGePoint3d& p2) const;
	bool ray(const AcGePoint3d& p1, const AcGePoint3d& p2) const;
	bool pline(const PyDbPolyline& lwBuf, unsigned int fromIndex, unsigned int numSegs) const;
	bool draw(PyGiDrawable& drawable) const;
	bool image(const PyGiImageBGRA32& img, const AcGePoint3d& position,
		const AcGeVector3d& u, const AcGeVector3d& v, int transparencyMode) const;
	bool rowOfDots(int count, const AcGePoint3d& start, const AcGeVector3d& step) const;
	bool ellipticalArc(const AcGePoint3d& center, const AcGeVector3d& normal,
		double majorAxisLength, double minorAxisLength,
		double startDegreeInRads, double endDegreeInRads, double tiltDegreeInRads, int arcType) const;
	bool pushClipBoundary(const std::vector<AcGePoint2d>& pts2d, const AcGeVector3d& normal,
		const AcGePoint3d& origin, bool clipFront, double frontZ, bool clipBack, double backZ,
		bool drawBoundary);
	void popClipBoundary();
	bool worldLine(const AcGePoint3d& p1, const AcGePoint3d& p2);
	bool polypoint(const std::vector<AcGePoint3d>& pts, const std::optional<AcGeVector3d>& normal) const;
};

class PyGiWorldGeometry : public PyGiGeometry
{
	PyWrapper_H(GiWorldGeometry)
public:
	PyGiWorldGeometry(AcGiWorldGeometry* ptr) :PyGiGeometry(ptr) {
	}
	virtual ~PyGiWorldGeometry() override = default;

	AcGiWorldGeometry* impWorld() const { return static_cast<AcGiWorldGeometry*>(impGeom()); }
	void setExtents(const AcGePoint3d& minPt, const AcGePoint3d& maxPt) const;
	void startAttributesSegment();
};


class PyGiViewportGeometry : public PyGiGeometry
{
	PyWrapper_H(GiViewportGeometry)
public:
	PyGiViewportGeometry(AcGiViewportGeometry* ptr) :PyGiGeometry(ptr) {}
	virtual ~PyGiViewportGeometry() override = default;

	AcGiViewportGeometry* impViewportGeom() const { return static_cast<AcGiViewportGeometry*>(impGeom()); }
	bool polylineEye(const std::vector<AcGePoint3d>& pts) const;
	bool polygonEye(const std::vector<AcGePoint3d>& pts) const;
	bool polylineDc(const std::vector<AcGePoint3d>& pts) const;
	bool polygonDc(const std::vector<AcGePoint3d>& pts) const;
};
#pragma pack (pop)
