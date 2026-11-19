#pragma once
#include "stdafx.h"
#include "PyDbObject.h"
#include "PyDbEntity.h"

class PyDbDictionary : public PyDbObject {
public:
	PyWrapper_H(DbDictionary)
	PyDbDictionary(AcDbDictionary* ptr) : PyDbObject(ptr) {}
	bool add(const AcString& name, PyDbObject& obj);
	py::object getAt(const AcString& name);   // 按类注册表还原真实类型（块表记录/XRecord 等）
	bool remove(const AcString& name);
	int numEntries() const;
	bool has(const AcString& name) const;
	// 按名称取条目 ObjectId（不打开对象），失败返回 0
	uint64_t getIdAt(const AcString& name) const;
	// 反向查找：ObjectId -> 条目名，失败返回空串
	AcString nameAt(uint64_t objId) const;
	// 按 ObjectId 删除条目
	bool removeById(uint64_t objId);
	// 按 ObjectId 判断条目是否存在
	bool hasId(uint64_t objId) const;
	bool rename(const AcString& oldName, const AcString& newName);
	// setAt(key, obj) -> (ok, newObjId)；透出新增对象 ObjectId
	std::tuple<bool, uint64_t> setAt(const AcString& key, PyDbObject& obj);
	AcDb::DuplicateRecordCloning mergeStyle() const;
	void setMergeStyle(int style);
	// ---- 容器协议：遍历 + 索引（迭代器折叠进字典本体）----
	py::list names() const;
	py::list values(int mode = 1);
	py::list items(int mode = 1);
	// 按位置取条目：(条目名, 对象)；负索引从末尾算，越界抛 IndexError
	std::tuple<AcString, PyDbObject> entryAt(int index, int mode = 1) const;
	bool isTreatElementsAsHard() const;
	void setTreatElementsAsHard(bool doIt);
	py::dict toDict();
};

void bind_PyDbDictionary(pym& m);
