#pragma once
#include "stdafx.h"
#include "PyDbObject.h"
#include "PyDbEntity.h"

class PyDbGroup : public PyDbObject {
public:
	PyWrapper_H(DbGroup)
	PyDbGroup(AcDbGroup* ptr) : PyDbObject(ptr) {}
	PyDbGroup() : PyDbObject(new AcDbGroup()) {}
	PyDbGroup(const AcString& desc, bool selectable) : PyDbObject(new AcDbGroup(desc.kwszPtr(), selectable)) {}
	bool addEntity(PyDbEntity& e);
	bool removeEntity(PyDbEntity& e);
	bool hasEntity(PyDbEntity& e) const;
	AcString getName() const;
	bool setName(const AcString& s);
	bool isAnonymous() const;
	bool isNotAccessible() const;
	bool clear();
	bool reverse();
	AcString description() const;
	bool setDescription(const AcString& s);
	int numEntities() const;
	bool isSelectable() const;
	bool setSelectable(bool selectable);

	// ---- 按 ObjectId（uint64）操作成员 ----
	py::list allEntityIds() const;
	int getIndex(uint64_t objId) const;
	bool removeAt(int index);
	bool replace(uint64_t oldId, uint64_t newId);
	bool prepend(uint64_t objId);
	bool insertAt(int index, uint64_t objId);

	// ---- 组级属性批量设置 ----
	bool setColorIndex(int colorIndex);
	bool setLayerByName(const AcString& layerName);
	bool setLinetypeByName(const AcString& ltypeName);
	bool setLinetypeScale(double scale);
	bool setVisibility(int visibility);
	bool setHighlight(bool highlight);

	// ---- 匿名化 / 材质 / 成员顺序调整 ----
	bool setAnonymous();
	bool setMaterial(const AcString& materialName);
	bool transfer(int fromIndex, int toIndex, int numItems);

	AcString toString() const;
};

void bind_PyDbGroup(pym& m);
