#pragma once
#include "stdafx.h"
#include <dbDetailViewStyle.h>
#include "PyDbModelDocViewStyle.h"

class PyDbDetailViewStyle : public PyDbModelDocViewStyle {
public:
	PyWrapper_H(DbDetailViewStyle)
	PyDbDetailViewStyle(AcDbDetailViewStyle* ptr) : PyDbModelDocViewStyle(ptr) {}
	PyDbDetailViewStyle() : PyDbDetailViewStyle(new AcDbDetailViewStyle()) {}

	std::tuple<int,int,int> arrowSymbolColor() const;
	uint64_t arrowSymbolId() const;
	double arrowSymbolSize() const;
	std::tuple<int,int,int> identifierColor() const;
	double identifierHeight() const;
	double identifierOffset() const;
	int identifierPlacement() const;
	uint64_t identifierStyleId() const;
	bool setArrowSymbolColor(int r, int g, int b);
	bool setArrowSymbolId(uint64_t id);
	bool setArrowSymbolSize(double size);
	bool setIdentifierColor(int r, int g, int b);
	bool setIdentifierHeight(double h);
	bool setIdentifierOffset(double v);
	bool setIdentifierPlacement(int placement);
	bool setIdentifierStyleId(uint64_t id);
};

void bind_PyDbDetailViewStyle(pym& m);
