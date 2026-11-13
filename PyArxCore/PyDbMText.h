#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include "PyColor.h"

// Read-only view of AcDbMTextFragment handed to the Python callback of
// explodeFragments. Fields are copied out of the ObjectARX struct during the
// enumeration (the native fragment pointer is only valid inside the callback).
struct PyDbMTextFragmentView {
	AcGePoint3d location;
	AcGeVector3d normal;
	AcGeVector3d direction;
	AcString text;
	AcString font;
	AcString bigFont;
	AcGePoint2d extents;
	double capsHeight = 0.0;
	double widthFactor = 0.0;
	double obliqueAngle = 0.0;
	double trackingFactor = 0.0;
	std::tuple<int, int, int> colorRGB{0, 0, 0};
	bool stackTop = false;
	bool stackBottom = false;
	bool underlined = false;
	bool overlined = false;
	bool strikethrough = false;
	AcGePoint3d underPoint1;
	AcGePoint3d underPoint2;
	AcGePoint3d overPoint1;
	AcGePoint3d overPoint2;
	AcGePoint3d strikePoint1;
	AcGePoint3d strikePoint2;
	AcString fontName;
	bool bold = false;
	bool italic = false;
};

class PyDbMText : public PyDbEntity {
	PyWrapper_H(DbMText)
public:
	PyDbMText(AcDbMText* p) : PyDbEntity(p) {}
	PyDbMText() : PyDbMText(new AcDbMText()) {}
	AcGePoint3d location() const;
	bool setLocation(const AcGePoint3d&);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	AcGeVector3d direction() const;
	bool setDirection(const AcGeVector3d&);
	double rotation() const;
	bool setRotation(double);
	double width() const;
	bool setWidth(double);
	double ascent() const;
	double descent() const;
	double textHeight() const;
	bool setTextHeight(double);
	int attachment() const;
	bool setAttachment(int);
	int flowDirection() const;
	bool setFlowDirection(int);
	AcString contents() const;
	bool setContents(const AcString&);
	AcString contentsRTF() const;
	bool setContentsRTF(const AcString&);
	AcString text() const;
	double actualHeight() const;
	double actualWidth() const;
	int lineSpacingStyle() const;
	bool setLineSpacingStyle(int);
	double lineSpacingFactor() const;
	bool setLineSpacingFactor(double);
	bool showBorders() const;
	bool setShowBorders(bool);
	bool backgroundFillOn() const;
	bool setBackgroundFill(bool);
	PyColor backgroundFillColor() const;
	bool setBackgroundFillColor(const PyColor& color);
	bool useBackgroundColorOn() const;
	bool setUseBackgroundColor(bool);
	bool useWordBreak() const;
	bool setUseWordBreak(bool);
	bool hitTest(const AcGePoint3d& pt) const;
	int correctSpelling();
	double height() const;
	bool setHeight(double);
	uint64_t textStyle() const;
	bool setTextStyle(uint64_t);
	std::vector<AcGePoint3d> getBoundingPoints() const;
	std::vector<std::vector<AcGePoint3d>> getBoundingPointsByLine() const;
	bool setAttachmentMovingLocation(int);
	py::tuple getPlane() const;
	bool isPlanar() const;
	void explodeFragments(const py::object& callback);
	AcString toString() const;
};

void bind_PyDbMText(pym& m);
