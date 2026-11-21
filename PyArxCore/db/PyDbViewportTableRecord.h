#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableRecord.h"

class PyDbViewportTableRecord : public PyDbSymbolTableRecord {
public:
	PyWrapper_H(DbViewportTableRecord)
	PyDbViewportTableRecord(AcDbViewportTableRecord* ptr) : PyDbSymbolTableRecord(ptr) {}
	PyDbViewportTableRecord() : PyDbViewportTableRecord(new AcDbViewportTableRecord()) {}

	// ---- AcDbViewportTableRecord own ----
	int number() const;
	AcGePoint2d lowerLeftCorner() const;
	bool setLowerLeftCorner(const AcGePoint2d& pt);
	AcGePoint2d upperRightCorner() const;
	bool setUpperRightCorner(const AcGePoint2d& pt);
	bool ucsFollowMode() const;
	bool setUcsFollowMode(bool enabled);
	int circleSides() const;
	bool setCircleSides(int sides);
	bool iconEnabled() const;
	bool setIconEnabled(bool enabled);
	bool iconAtOrigin() const;
	bool setIconAtOrigin(bool atOrigin);
	bool gridEnabled() const;
	bool setGridEnabled(bool enabled);
	AcGePoint2d gridIncrements() const;
	bool setGridIncrements(const AcGePoint2d& pt);
	bool snapEnabled() const;
	bool setSnapEnabled(bool enabled);
	bool isometricSnapEnabled() const;
	bool setIsometricSnapEnabled(bool enabled);
	int snapPair() const;
	bool setSnapPair(int pairType);
	double snapAngle() const;
	bool setSnapAngle(double angle);
	AcGePoint2d snapBase() const;
	bool setSnapBase(const AcGePoint2d& pt);
	AcGePoint2d snapIncrements() const;
	bool setSnapIncrements(const AcGePoint2d& pt);
	bool isUcsSavedWithViewport() const;
	bool setUcsPerViewport(bool ucsvp);
	bool isGridBoundToLimits() const;
	bool setGridBoundToLimits(bool enabled);
	bool isGridAdaptive() const;
	bool setGridAdaptive(bool enabled);
	bool isGridSubdivisionRestricted() const;
	bool setGridSubdivisionRestricted(bool enabled);
	bool isGridFollow() const;
	bool setGridFollow(bool enabled);
	int gridMajor() const;
	bool setGridMajor(int value);

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

	// ---- 背景与快速缩放（AcDbViewportTableRecord 自己）----
	bool fastZoomsEnabled() const;
	uint64_t previousBackground(int type) const;
	bool previousBackgroundForcedSwitch() const;
	bool setBackground(uint64_t backgroundId);
	bool setFastZoomsEnabled(bool enabled);
	bool setPreviousBackground(uint64_t backgroundId, int type, bool bForcedSwitch);

	// ---- 未绑定（指针类型无对应 Python 封装）----
	// setGsView(AcGsView*), drawable()
};

void bind_PyDbViewportTableRecord(pym& m);
