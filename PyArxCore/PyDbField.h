#pragma once
#include "stdafx.h"
#include <dbField.h>
#include "PyDbObject.h"

class PyDbDatabase;

void bind_PyDbField(pym& m);

// PyRx Db.Field 的 PyArx 实现：AcDbField 包装类（32 成员）
class PyDbField : public PyDbObject {
	PyWrapper_H(DbField)
public:
	PyDbField(AcDbField* ptr) : PyDbObject(ptr) {}
	virtual ~PyDbField(void) override = default;

	// 构造：字段码 / (字段码, 是否文本字段)
	static PyDbField* create(const AcString& fieldCode, bool bTextField);

	// 属性
	int   state() const;
	int   evaluationStatus() const;
	int   evaluationOption() const;
	bool  setEvaluationOption(int option);
	int   filingOption() const;
	bool  setFilingOption(int option);
	AcString evaluatorId() const;
	bool  setEvaluatorId(const AcString& evaluatorId);
	bool  isTextField() const;
	bool  convertToTextField();
	int   dataType() const;
	int   childCount() const;

	// 字段码 / 格式 / 值
	AcString getFieldCode(int flag) const;
	bool  setFieldCode(const AcString& fieldCode, int flag);
	AcString getFormat() const;
	bool  setFormat(const AcString& format);
	AcString getValue() const;

	// 求值
	py::tuple evaluate(int context, PyDbDatabase* db);
	PyDbField* getChild(int index, AcDb::OpenMode mode);

	// 落库
	bool setInObject(PyDbObject& obj, const AcString& propName);
	bool postInDatabase(PyDbDatabase& db);
};
