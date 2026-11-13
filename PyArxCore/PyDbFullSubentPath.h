#pragma once 
#include "stdafx.h"

class PyRxClass;



class PyDbSubentId
{
public:
	// ============= 构造与析构 =============
	PyDbSubentId();
	explicit PyDbSubentId(AcDbSubentId* pSubentId, bool isOwner = false);
	explicit PyDbSubentId(const AcDbSubentId& subId);
	explicit PyDbSubentId(AcDbSubentId& subId);
	PyDbSubentId(AcDb::SubentType type, Adesk::GsMarker index);
	PyDbSubentId(PyRxClass& pTypeClass, Adesk::GsMarker index);

	// 拷贝构造和赋值
	PyDbSubentId(const PyDbSubentId& other);
	PyDbSubentId& operator=(const PyDbSubentId& other);

	~PyDbSubentId();
	operator AcDbSubentId() const;
	bool operator==(const PyDbSubentId& other) const;
	bool operator!=(const PyDbSubentId& other) const;

	AcDb::SubentType type() const;
	bool setType(AcDb::SubentType);
	PyRxClass typeClass() const;
	bool setTypeClass(PyRxClass& pClass);
	Adesk::GsMarker index() const;
	bool setIndex(Adesk::GsMarker);
	static const PyDbSubentId kNull;
private:
	AcDbSubentId* m_ptr;
	bool m_isOwner;
};

class PyDbFullSubentPath
{
public:
	PyDbFullSubentPath();
	explicit PyDbFullSubentPath(const AcDbFullSubentPath& fsp);
	PyDbFullSubentPath(AcDb::SubentType type, Adesk::GsMarker index);
	PyDbFullSubentPath(AcDbObjectId entId, AcDb::SubentType type, Adesk::GsMarker index);
	PyDbFullSubentPath(AcDbObjectId entId, AcDbSubentId subId);
	PyDbFullSubentPath(const AcDbObjectIdArray& objectIds, AcDbSubentId subId);

	// 拷贝构造和赋值 
	PyDbFullSubentPath(const PyDbFullSubentPath& other);
	PyDbFullSubentPath& operator=(const PyDbFullSubentPath& other);

	operator AcDbFullSubentPath() const;

	bool operator==(const PyDbFullSubentPath& other) const;
	bool operator!=(const PyDbFullSubentPath& other) const;

	bool setObjectIds(const std::vector<AcDbObjectId>& objectIds);
	std::vector<AcDbObjectId> objectIds() const;
	bool setSubentId(const PyDbSubentId& subentId);
	PyDbSubentId subentId() const;
	PyDbSubentId subentId();

	static const AcDbFullSubentPath kNull;
	AcDbFullSubentPath& impObj() {
		return m_fsp;
	}
	AcDbFullSubentPath impObj() const {
		return m_fsp;
	}

private:
	AcDbFullSubentPath m_fsp;
};

void bind_PyDbSubentId(pym& m);
void bind_PyDbFullSubentPath(pym& m);