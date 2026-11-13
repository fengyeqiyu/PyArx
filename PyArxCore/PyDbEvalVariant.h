#pragma once
#include "stdafx.h"
#include "dbeval.h"

// AcDbEvalVariant : public resbuf, public AcRxObject  (dbeval.h:99)
// 值容器（非 DB 驻留），按值持有，仿照 PyDbHandle 的值类模式。
class PyDbEvalVariant {
public:
	PyDbEvalVariant();
	explicit PyDbEvalVariant(double v);
	explicit PyDbEvalVariant(short v);
	explicit PyDbEvalVariant(Adesk::Int32 v);
	explicit PyDbEvalVariant(const AcString& v);
	explicit PyDbEvalVariant(uint64_t id);
	explicit PyDbEvalVariant(const AcGePoint2d& pt);
	explicit PyDbEvalVariant(const AcGePoint3d& pt);
	// 由 SDK 返回值构造（AcDbEvalExpr::value() 按值返回）
	explicit PyDbEvalVariant(const AcDbEvalVariant& other);

	AcDbEvalVariant& impObj();
	const AcDbEvalVariant& impObj() const;

private:
	AcDbEvalVariant m_val;
};

void bind_PyDbEvalVariant(pym& m);
