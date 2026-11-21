#pragma once
#include "stdafx.h"

#include <tuple>

// SDK: dbidmap.h — class AcDbIdMappingIter : public AcRxObject
// 持有者 PyDbIdMapping 已封装（P1）。SDK ctor: AcDbIdMappingIter(const AcDbIdMapping&)。
// 当前元素 getMap() 得 AcDbIdPair，以 (key, value) 的 uint64 旧 id 元组透出。
class AcDbIdMapping;
class AcDbIdMappingIter;

class PyDbIdMappingIter {
	AcDbIdMappingIter* m_p = nullptr;
public:
	// 从持有者创建（本包装接管所有权，析构 delete）。
	explicit PyDbIdMappingIter(const AcDbIdMapping* pOwner);
	// 指针接管：析构 delete。
	explicit PyDbIdMappingIter(AcDbIdMappingIter* p) : m_p(p) {}
	~PyDbIdMappingIter();
	PyDbIdMappingIter(const PyDbIdMappingIter&) = delete;
	PyDbIdMappingIter& operator=(const PyDbIdMappingIter&) = delete;

	bool done();
	bool step();

public:
	AcDbIdMappingIter* impObj() { return m_p; }
};

void bind_PyDbIdMappingIter(pym& m);
