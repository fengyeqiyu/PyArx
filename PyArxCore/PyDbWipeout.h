#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include <dbwipe.h>

class PyDbWipeout : public PyDbEntity {
	PyWrapper_H(DbWipeout)
public:
	PyDbWipeout(AcDbWipeout* p) : PyDbEntity(p) {}
	PyDbWipeout() : PyDbWipeout(new AcDbWipeout()) {}
	bool frame() const;
	bool isClipped() const;
	int brightness() const;
	bool setBrightness(int value);
	int contrast() const;
	bool setContrast(int value);
	int fade() const;
	bool setFade(int value);
	AcGeVector2d scale() const;
	AcGeVector2d imageSize(bool cached = false) const;
	bool append(uint64_t id);
	AcString toString() const;
};

void bind_PyDbWipeout(pym& m);
