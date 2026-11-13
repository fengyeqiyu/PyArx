#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbViewport : public PyDbEntity {
	PyWrapper_H(DbViewport)
public:
	PyDbViewport(AcDbViewport* p) : PyDbEntity(p) {}
	PyDbViewport() : PyDbViewport(new AcDbViewport()) {}
	double height() const;
	bool setHeight(double);
	double width() const;
	bool setWidth(double);
	AcGePoint3d centerPoint() const;
	bool setCenterPoint(const AcGePoint3d&);
	AcGePoint3d viewTarget() const;
	bool setViewTarget(const AcGePoint3d&);
	int number() const;
	bool isOn() const;
	bool setIsOn(bool);
	AcGeVector3d viewDirection() const;
	bool setViewDirection(const AcGeVector3d&);
	double viewHeight() const;
	bool setViewHeight(double);
	AcGePoint2d viewCenter() const;
	bool setViewCenter(const AcGePoint2d&);
	double twistAngle() const;
	bool setTwistAngle(double);
	double lensLength() const;
	bool setLensLength(double);
	bool isFrontClipOn() const;
	bool setFrontClipOn(bool);
	bool isBackClipOn() const;
	bool setBackClipOn(bool);
	double frontClipDistance() const;
	bool setFrontClipDistance(double);
	double backClipDistance() const;
	bool setBackClipDistance(double);
	bool isPerspectiveOn() const;
	bool setPerspectiveOn(bool);
	int circleSides() const;
	bool setCircleSides(int);
	bool isSnapOn() const;
	bool setSnapOn(bool);
	double snapAngle() const;
	bool setSnapAngle(double);
	AcGePoint2d snapBasePoint() const;
	bool setSnapBasePoint(const AcGePoint2d&);
	AcGeVector2d snapIncrement() const;
	bool setSnapIncrement(const AcGeVector2d&);
	bool isGridOn() const;
	bool setGridOn(bool);
	int gridMajor() const;
	bool setGridMajor(int);
	AcGeVector2d gridIncrement() const;
	bool setGridIncrement(const AcGeVector2d&);
	bool hiddenLinesRemoved() const;
	bool removeHiddenLines(bool);
	bool isLocked() const;
	bool setLocked();
	bool setUnlocked();
	bool updateDisplay();
	// ---- 显示/裁剪/UCS 开关与视图关联（AcDbViewport 自己）----
	bool isFastZoomOn() const;
	bool isFrontClipAtEyeOn() const;
	bool isSnapIsometric() const;
	bool isUcsFollowModeOn() const;
	bool isUcsIconAtOrigin() const;
	bool isUcsIconVisible() const;
	uint64_t getLabelBlock() const;
	uint64_t getSheetView() const;
	bool removeLabelBlock();
	bool removeModelView();
	bool removeSheetView();
	bool setBackClipOff();
	bool setFastZoomOff();
	bool setFastZoomOn(bool bOn);
	bool setFrontClipAtEyeOff();
	bool setFrontClipAtEyeOn(bool bOn);
	bool setFrontClipOff();
	bool setLabelBlock(uint64_t objId);
	bool setOff();
	bool setOn();
	bool setPerspectiveOff();
	bool setSheetView(uint64_t objId);
	bool setSnapIsometric(bool bOn);
	bool setSnapOff();
	bool setUcsFollowModeOff();
	bool setUcsFollowModeOn(bool bOn);
	bool setUcsIconAtCorner();
	bool setUcsIconAtOrigin(bool bOn);
	bool setUcsIconInvisible();
	bool setUcsIconVisible(bool bOn);
	bool syncModelView();
	AcString toString() const;
};

void bind_PyDbViewport(pym& m);
