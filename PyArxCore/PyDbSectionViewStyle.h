#pragma once
#include "stdafx.h"
#include <dbSectionViewStyle.h>
#include "PyDbModelDocViewStyle.h"

class PyDbSectionViewStyle : public PyDbModelDocViewStyle {
public:
	PyWrapper_H(DbSectionViewStyle)
	PyDbSectionViewStyle(AcDbSectionViewStyle* ptr) : PyDbModelDocViewStyle(ptr) {}
	PyDbSectionViewStyle() : PyDbSectionViewStyle(new AcDbSectionViewStyle()) {}

	uint64_t arrowEndSymbolId() const;
	uint64_t arrowStartSymbolId() const;
	std::tuple<int,int,int> arrowSymbolColor() const;
	double arrowSymbolSize() const;
	std::tuple<int,int,int> identifierColor() const;
	double identifierHeight() const;
	uint64_t identifierStyleId() const;
	bool setArrowEndSymbolId(uint64_t id);
	bool setArrowStartSymbolId(uint64_t id);
	bool setArrowSymbolColor(int r, int g, int b);
	bool setArrowSymbolSize(double size);
	bool setIdentifierColor(int r, int g, int b);
	bool setIdentifierExcludeCharacters(const AcString& s);
	bool setIdentifierHeight(double h);
	bool setIdentifierStyleId(uint64_t id);
};

void bind_PyDbSectionViewStyle(pym& m);
