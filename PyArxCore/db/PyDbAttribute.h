#pragma once
#include "stdafx.h"
#include "PyDbText.h"

class PyDbMText;
class PyDbAttributeDefinition;

class PyDbAttribute : public PyDbText {
	PyWrapper_H(DbAttribute)
public:
	PyDbAttribute(AcDbAttribute* p) : PyDbText(p) {}
	PyDbAttribute() : PyDbAttribute(new AcDbAttribute()) {}
	PyDbAttribute(const AcGePoint3d& position, const AcString& text, const AcString& tag)
		: PyDbText(new AcDbAttribute(position, text.kwszPtr(), tag.kwszPtr(), AcDbObjectId::kNull)) {}
	AcString tag() const;
	bool setTag(const AcString&);
	bool isInvisible() const;
	bool setInvisible(bool);
	bool isConstant() const;
	bool isVerifiable() const;
	bool isPreset() const;
	int fieldLength() const;
	bool setFieldLength(int);
	bool lockPositionInBlock() const;
	bool setLockPositionInBlock(bool);
	bool isReallyLocked() const;
	// ---- 多行属性（MText）支持 ----
	bool convertIntoMTextAttribute(bool val = true);
	bool isMTextAttribute() const;
	bool setAttributeFromBlock(const AcGeMatrix3d& blkXform);
	bool setAttributeFromBlock(const PyDbAttributeDefinition* pAttdef, const AcGeMatrix3d& blkXform);
	bool setMTextAttribute(PyDbMText* pMText);
	bool setMTextAttributeConst(const PyDbMText* pMText);
	bool updateMTextAttribute();
	PyDbMText getMTextAttribute() const;
	PyDbMText getMTextAttributeConst() const;
	AcString toString() const;
};

void bind_PyDbAttribute(pym& m);
