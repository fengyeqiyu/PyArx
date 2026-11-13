#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbRasterImage : public PyDbEntity {
	PyWrapper_H(DbRasterImage)
public:
	PyDbRasterImage(AcDbRasterImage* p) : PyDbEntity(p) {}
	PyDbRasterImage() : PyDbRasterImage(new AcDbRasterImage()) {}
	AcGePoint3d position() const;
	double rotation() const;
	bool setRotation(double);
	uint64_t imageDefId() const;
	AcGeVector2d imageSize() const;
	AcGeVector2d scale() const;
	double width() const;
	double height() const;
	double imageWidth() const;
	double imageHeight() const;
	bool setWidth(double);
	bool setHeight(double);
	int brightness() const;
	bool setBrightness(int);
	int contrast() const;
	bool setContrast(int);
	int fade() const;
	bool setFade(int);
	bool isClipped() const;
	bool isClipInverted() const;
	void setClipInverted(bool);
	bool isImageShown() const;
	void setShowImage(bool);
	bool isImageTransparent() const;
	void setImageTransparency(bool);
	bool isShownClipped() const;
	void setShowClipped(bool);
	bool castShadows() const;
	void setCastShadows(bool);
	bool receiveShadows() const;
	void setReceiveShadows(bool);
	int clipBoundaryType() const;
	AcString toString() const;
};

void bind_PyDbRasterImage(pym& m);
