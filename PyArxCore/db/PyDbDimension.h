#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

// Abstract dimension base (pointer ctor only); shared accessors.
class PyDbDimension : public PyDbEntity {
	PyWrapper_H(DbDimension)
public:
	PyDbDimension(AcDbDimension* p) : PyDbEntity(p) {}
	AcGePoint3d textPosition() const;
	bool setTextPosition(const AcGePoint3d&);
	AcString dimensionText() const;
	bool setDimensionText(const AcString&);
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	double elevation() const;
	bool setElevation(double);
	double textRotation() const;
	bool setTextRotation(double);
	double horizontalRotation() const;
	bool setHorizontalRotation(double);
	double measurement();
	bool isUsingDefaultTextPosition() const;
	bool useSetTextPosition();
	bool useDefaultTextPosition();
	bool recomputeDimBlock(bool forceUpdate = true);
	bool generateLayout();
	bool isDynamicDimension() const;
	bool setDynamicDimension(bool);

	// ---- 标注样式 ID ----
	uint64_t dimensionStyle() const;
	bool setDimensionStyle(uint64_t styleId);

	// ---- 默认文字位置开关 ----
	bool setUsingDefaultTextPosition(bool v);

	// ---- 文字附着方式（AcDbMText::AttachmentPoint，int 透出）----
	int  textAttachment() const;
	bool setTextAttachment(int v);

	// ---- 前缀 / 后缀 ----
	AcString prefix() const;
	bool setPrefix(const AcString& s);
	AcString suffix() const;
	bool setSuffix(const AcString& s);

	// ---- 箭头翻转 ----
	bool getArrowFirstIsFlipped() const;
	bool getArrowSecondIsFlipped() const;
	bool setArrowFirstIsFlipped(bool v);
	bool setArrowSecondIsFlipped(bool v);

	// ---- 高频标注变量覆写（dbdimvar.h）----
	double dimtxt() const;
	bool   setDimtxt(double v);
	double dimgap() const;
	bool   setDimgap(double v);
	double dimasz() const;
	bool   setDimasz(double v);
	int    dimdec() const;
	bool   setDimdec(int v);
	bool   dimtol() const;
	bool   setDimtol(bool v);
	double dimtp() const;
	bool   setDimtp(double v);
	double dimtm() const;
	bool   setDimtm(double v);
	double dimscale() const;
	bool   setDimscale(double v);
	uint64_t dimtxsty() const;
	bool   setDimtxsty(uint64_t id);
	bool   dimse1() const;
	bool   setDimse1(bool v);
	bool   dimse2() const;
	bool   setDimse2(bool v);
	double dimexe() const;
	bool   setDimexe(double v);
	double dimexo() const;
	bool   setDimexo(double v);
	int    dimtad() const;
	bool   setDimtad(int v);

	AcString toString() const;
};

void bind_PyDbDimension(pym& m);
