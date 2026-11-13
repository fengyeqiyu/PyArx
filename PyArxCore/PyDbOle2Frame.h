#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbOle2Frame : public PyDbEntity {
	PyWrapper_H(DbOle2Frame)
public:
	PyDbOle2Frame(AcDbOle2Frame* p) : PyDbEntity(p) {}
	PyDbOle2Frame() : PyDbOle2Frame(new AcDbOle2Frame()) {}
	AcGePoint3d location() const;
	void setLocation(const AcGePoint3d&);
	// 四角世界坐标：返回 (upLeft, upRight, lowLeft, lowRight)
	std::tuple<AcGePoint3d, AcGePoint3d, AcGePoint3d, AcGePoint3d> position3d() const;
	void setPosition3d(const AcGePoint3d& upLeft, const AcGePoint3d& upRight,
		const AcGePoint3d& lowLeft, const AcGePoint3d& lowRight);
	double rotation() const;
	bool setRotation(double);
	int getType() const;
	AcString getUserType() const;
	AcString getLinkName() const;
	AcString getLinkPath() const;
	bool isLinked() const;
	int outputQuality() const;
	void setOutputQuality(int);
	bool autoOutputQuality() const;
	void setAutoOutputQuality(int);
	double wcsWidth() const;
	bool setWcsWidth(double);
	double wcsHeight() const;
	bool setWcsHeight(double);
	double scaleWidth() const;
	bool setScaleWidth(double);
	double scaleHeight() const;
	bool setScaleHeight(double);
	bool lockAspect() const;
	bool setLockAspect(bool);
	AcString toString() const;
};

void bind_PyDbOle2Frame(pym& m);
