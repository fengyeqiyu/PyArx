#pragma once
#include "stdafx.h"
#include "PyDbText.h"

class PyDbMText;
class PyDbDatabase;

class PyDbAttributeDefinition : public PyDbText {
	PyWrapper_H(DbAttributeDefinition)
public:
	PyDbAttributeDefinition(AcDbAttributeDefinition* p) : PyDbText(p) {}
	PyDbAttributeDefinition() : PyDbAttributeDefinition(new AcDbAttributeDefinition()) {}
	PyDbAttributeDefinition(const AcGePoint3d& position, const AcString& text, const AcString& tag, const AcString& prompt)
		: PyDbText(new AcDbAttributeDefinition(position, text.kwszPtr(), tag.kwszPtr(), prompt.kwszPtr(), AcDbObjectId::kNull)) {}
	AcString tag() const;
	bool setTag(const AcString&);
	AcString prompt() const;
	bool setPrompt(const AcString&);
	bool isInvisible() const;
	bool setInvisible(bool);
	bool isConstant() const;
	bool setConstant(bool);
	bool isVerifiable() const;
	bool setVerifiable(bool);
	bool isPreset() const;
	bool setPreset(bool);
	int fieldLength() const;
	bool setFieldLength(int);
	bool lockPositionInBlock() const;
	bool setLockPositionInBlock(bool);
	// ---- 多行属性定义（MText）支持 ----
	bool adjustAlignment(const PyDbDatabase* pDb);
	bool convertIntoMTextAttributeDefinition(bool val = true);
	bool isMTextAttributeDefinition() const;
	bool setMTextAttributeDefinition(PyDbMText* pMText);
	bool setMTextAttributeDefinitionConst(const PyDbMText* pMText);
	bool updateMTextAttributeDefinition();
	PyDbMText getMTextAttributeDefinition() const;
	PyDbMText getMTextAttributeDefinitionConst() const;
	AcString toString() const;
};

void bind_PyDbAttributeDefinition(pym& m);
