#pragma once
#include "stdafx.h"
#include "PyDbObject.h"

class PyDbLayout : public PyDbObject {
public:
	PyWrapper_H(DbLayout)
	PyDbLayout(AcDbLayout* ptr) : PyDbObject(ptr) {}
	PyDbLayout() : PyDbObject(new AcDbLayout()) {}
	uint64_t getBlockTableRecordId() const;
	bool setBlockTableRecordId(uint64_t btrId);
	AcString getLayoutName() const;
	bool setLayoutName(const AcString& name);
	int getTabOrder() const;
	void setTabOrder(int order);
	bool getTabSelected() const;
	void setTabSelected(bool selected);

	// ---- 视口数组 / 范围 / 注释可见性 ----
	py::list getViewportArray() const;
	py::tuple getLimits() const;
	py::tuple getExtents() const;
	bool annoAllVisible() const;
	bool setAnnoAllVisible(bool visible);

	AcString toString() const;
};

void bind_PyDbLayout(pym& m);
