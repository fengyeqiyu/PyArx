#pragma once
#include "stdafx.h"
#include <dbdimassoc.h>
#include "PyDbObject.h"

class PyDbDimAssoc : public PyDbObject {
public:
	PyWrapper_H(DbDimAssoc)
	PyDbDimAssoc(AcDbDimAssoc* ptr) : PyDbObject(ptr) {}
	PyDbDimAssoc() : PyDbDimAssoc(new AcDbDimAssoc()) {}

	bool addToDimensionReactor(bool isAdd);
	bool addToPointRefReactor();
	int assocFlag();
	bool assocFlag(int ptType) const;
	uint64_t dimObjId() const;
	AcDbObjectIdArray getDimAssocGeomIds() const;
	bool hasOopsWatcher() const;
	bool isAllGeomErased() const;
	bool isTransSpatial() const;
	std::tuple<bool, uint64_t> post(uint64_t dimId, bool isActive);
	bool removeAssociativity(bool force);
	void removeOopsWatcher();
	bool removePointRef(int ptType);
	bool restoreAssocFromOopsWatcher();
	int rotatedDimType() const;
	void setAssocFlag(int assocFlg);
	bool setAssocFlag(int ptType, bool value);
	bool setDimObjId(uint64_t dimId);
	bool setRotatedDimType(int dimType);
	bool setTransSpatial(bool value);
	bool startCmdWatcher();
	bool startOopsWatcher(bool bAddAll);
	bool updateAssociativity(const AcDbObjectIdArray& ids);
	void updateDueToMirror(bool wasInMirror);
	bool updateFillet(const AcDbObjectIdArray& ids);
	bool updateXrefSubentPath();
};

void bind_PyDbDimAssoc(pym& m);
