#pragma once

//PyRxClass.h
#include "stdafx.h"
#include "rxclass.h"
#include "rxattrib.h"
#include "PyRxObject.h"


class PyRxClass : public PyRxObject
{
	PyWrapper_H(RxClass)

public:

	PyRxClass(AcRxClass* ptr = nullptr, bool autoDelete = false) :PyRxObject(ptr, autoDelete, false) {}

	virtual ~PyRxClass() override = default;
	int proxyFlags() const;
	bool isDerivedFrom(const PyRxClass& other) const;

	PyRxObject addX(PyRxClass& rxclass, PyRxObject& rxobj);
	PyRxObject getX(const PyRxClass& rxobj);
	PyRxObject delX(PyRxClass& rxclass);
	PyRxObject queryX(const PyRxClass& rxclass);
	PyRxObject queryX(const PyRxClass& rxclass) const;
	PyRxObject create();
	PyRxClass myParent() const;
};





