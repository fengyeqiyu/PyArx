#pragma once
#include "stdafx.h"
#include "PyDbObject.h"

class PyDbMlineStyle : public PyDbObject {
public:
	PyWrapper_H(DbMlineStyle)
	PyDbMlineStyle(AcDbMlineStyle* ptr) : PyDbObject(ptr) {}
	PyDbMlineStyle() : PyDbObject(new AcDbMlineStyle()) {}

	AcString description() const;
	bool setDescription(const AcString& desc);
	// Instance name() collides with the static PyWrapper_H name(); expose as styleName.
	AcString styleName() const;
	bool setStyleName(const AcString& name);
	bool showMiters() const;
	bool setShowMiters(bool show);
	bool startSquareCap() const;
	bool setStartSquareCap(bool show);
	bool startRoundCap() const;
	bool setStartRoundCap(bool show);
	bool startInnerArcs() const;
	bool setStartInnerArcs(bool show);
	bool endSquareCap() const;
	bool setEndSquareCap(bool show);
	bool endRoundCap() const;
	bool setEndRoundCap(bool show);
	bool endInnerArcs() const;
	bool setEndInnerArcs(bool show);
	bool filled() const;
	bool setFilled(bool onOff);
	double startAngle() const;
	bool setStartAngle(double ang);
	double endAngle() const;
	bool setEndAngle(double ang);
	int numElements() const;

	// ---- 新增 ----
	int addElement(double offset, int r, int g, int b, uint64_t linetypeId, bool checkIfReferenced = true);
	std::tuple<double, int, int, int, uint64_t> getElementAt(int elem) const;
	bool setElement(int elem, double offset, int r, int g, int b, uint64_t linetypeId);
	bool removeElementAt(int elem);
	bool setFillColor(int r, int g, int b);
	std::tuple<int, int, int> fillColor() const;
	void initMlineStyle();
	bool setFrom(const PyDbMlineStyle& src, bool checkIfReferenced = true);
	AcString toString() const;
};

void bind_PyDbMlineStyle(pym& m);
