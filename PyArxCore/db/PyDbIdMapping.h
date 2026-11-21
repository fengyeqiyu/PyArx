#pragma once
#include "stdafx.h"
#include "dbidmap.h"

class PyDbDatabase;

// AcDbIdMapping pointer wrapper (AcRxObject-derived, non-copyable; used by
// deepCloneObjects / wblockCloneObjects). Ids cross the Python boundary as
// uint64 old-style ids (0 == null), matching the rest of the project.
class PyDbIdMapping {
public:
	// Non-owning wrapper around an engine-/caller-owned mapping.
	explicit PyDbIdMapping(AcDbIdMapping* p = nullptr) : m_ptr(p), m_own(false) {}
	// Owned ctor (used by create() via make_unique; kept public so make_unique
	// can instantiate it).
	PyDbIdMapping(AcDbIdMapping* p, bool own) : m_ptr(p), m_own(own) {}
	~PyDbIdMapping();
	PyDbIdMapping(const PyDbIdMapping&) = delete;
	PyDbIdMapping& operator=(const PyDbIdMapping&) = delete;

	// Owned factory: allocates a new empty AcDbIdMapping.
	static std::unique_ptr<PyDbIdMapping> create();

	AcDbIdMapping* impObj() const { return m_ptr; }
	bool isNull() const { return m_ptr == nullptr; }

	// AcDbIdPair ops; ids are uint64 old-style ids.
	bool assign(uint64_t keyId, uint64_t valueId, bool isCloned,
		bool isPrimary = false, bool isOwnerXlated = true);
	// Look up keyId; returns the mapped value old-style id (0 when absent).
	uint64_t compute(uint64_t keyId) const;
	bool change(uint64_t keyId, uint64_t valueId, bool isCloned,
		bool isPrimary = false, bool isOwnerXlated = true);
	bool del(uint64_t keyId);

	bool setDestDb(PyDbDatabase& db);

	// 由其它 IdMapping 整体拷贝内容；失败返回 false
	bool copyFrom(const PyDbIdMapping& other);
	// 返回目标/原始数据库（失败返回空 PyDbDatabase）
	PyDbDatabase destDb() const;
	PyDbDatabase origDb() const;

	int deepCloneContext() const;
	int duplicateRecordCloning() const;
	AcString toString() const;
private:
	AcDbIdMapping* m_ptr = nullptr;
	bool m_own = false;
};

void bind_PyDbIdMapping(pym& m);
