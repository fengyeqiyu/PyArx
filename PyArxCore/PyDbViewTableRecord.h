#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableRecord.h"
#include "PyDbObject.h"
#include "PyDbAnnotationScale.h"

class PyDbViewTableRecord : public PyDbSymbolTableRecord {
public:
	PyWrapper_H(DbViewTableRecord)
	PyDbViewTableRecord(AcDbViewTableRecord* ptr) : PyDbSymbolTableRecord(ptr) {}

	// ---- AcDbViewTableRecord own ----
	bool isPaperspaceView() const;
	bool setIsPaperspaceView(bool pspace);
	bool isUcsAssociatedToView() const;
	bool disassociateUcsFromView();
	AcString getCategoryName() const;
	bool setCategoryName(const AcString& name);
	AcString getLayerState() const;
	bool setLayerState(const AcString& name);
	uint64_t getLayout() const;
	bool setLayout(uint64_t layoutId);
	bool isViewAssociatedToViewport() const;
	bool setViewAssociatedToViewport(bool flag);
	bool isCameraPlottable() const;
	bool setIsCameraPlottable(bool plottable);

	// ---- High-freq getters/setters inherited from AcDbAbstractViewTableRecord ----
	AcGePoint2d centerPoint() const;
	bool setCenterPoint(const AcGePoint2d& pt);
	double height() const;
	bool setHeight(double h);
	double width() const;
	bool setWidth(double w);
	AcGePoint3d target() const;
	bool setTarget(const AcGePoint3d& t);
	AcGeVector3d viewDirection() const;
	bool setViewDirection(const AcGeVector3d& v);
	double viewTwist() const;
	bool setViewTwist(double angle);
	double lensLength() const;
	bool setLensLength(double length);
	bool perspectiveEnabled() const;
	bool setPerspectiveEnabled(bool enabled);

	// ---- 裁剪平面与高程（继承自 AcDbAbstractViewTableRecord）----
	double frontClipDistance() const;
	bool setFrontClipDistance(double d);
	double backClipDistance() const;
	bool setBackClipDistance(double d);
	bool frontClipEnabled() const;
	bool setFrontClipEnabled(bool enabled);
	bool backClipEnabled() const;
	bool setBackClipEnabled(bool enabled);
	double elevation() const;
	bool setElevation(double e);

	// ---- 相机 / 活截面 / 太阳 / 注释比例（本类自有；ObjectId 以 uint64 透出）----
	uint64_t camera() const;
	uint64_t liveSection() const;
	uint64_t sunId() const;
	bool setCamera(uint64_t cameraId);
	bool setLiveSection(uint64_t liveSectionId);
	bool setParametersFromViewport(uint64_t viewportId);
	bool setAnnotationScale(PyDbAnnotationScale& scale);
	std::tuple<bool, uint64_t> setSun(PyDbObject& sunObj);
	std::tuple<bool, uint64_t> setSun(PyDbObject& sunObj, bool eraseOldSun);
	// ---- 未绑定（裸指针/未封装类型）----
	// getPreviewImage/setPreviewImage (Atil::Image* 出入参),
	// getThumbnail/setThumbnail (BITMAPINFO* 出入参), annotationScale() getter（调用方需 delete）。
};

void bind_PyDbViewTableRecord(pym& m);
