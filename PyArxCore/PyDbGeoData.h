#pragma once
#include "stdafx.h"
#include <dbGeoData.h>
#include "PyDbObject.h"

class PyDbGeoData : public PyDbObject {
public:
	PyWrapper_H(DbGeoData)
	PyDbGeoData(AcDbGeoData* ptr) : PyDbObject(ptr) {}
	PyDbGeoData() : PyDbGeoData(new AcDbGeoData()) {}

	uint64_t blockTableRecordId() const;
	int coordinateType() const;
	AcGePoint3d designPoint() const;
	bool eraseFromDb();
	std::tuple<bool, uint64_t> postToDb();
	bool setBlockTableRecordId(uint64_t blockId);
	bool setCoordinateType(int t);
	bool setDesignPoint(const AcGePoint3d& p);
};

void bind_PyDbGeoData(pym& m);
