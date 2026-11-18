//PyRxClass.cpp
#include "stdafx.h"
#include "PyRxObject.h"
#include "stdafx.h"
#include "PyRxClass.h"

PyWrapper_Cpp(RxClass)



bool PyRxClass::isDerivedFrom(const PyRxClass& otherClass) const
{
	if (!impObj() || !otherClass.impObj()) return false;
	return AcRxClass::isDerivedFrom(impObj()->isA(), otherClass.impObj());
}


int PyRxClass::proxyFlags() const
{
	return impObj() ? impObj()->proxyFlags() : 0;
}


PyRxObject PyRxClass::addX(PyRxClass& rxclass, PyRxObject& rxobj)
{
	if (!impObj() || !rxclass.impObj() || !rxobj.impObj()) return PyRxObject();
	return impObj()->addX(rxclass.impObj(), rxobj.impObj());
}

PyRxObject PyRxClass::getX(const PyRxClass& rxobj)
{
	if (!impObj() || !rxobj.impObj()) return PyRxObject();
	return impObj()->getX(rxobj.impObj());
}

PyRxObject PyRxClass::delX(PyRxClass& rxclass)
{
	if (!impObj() || !rxclass.impObj()) return PyRxObject();
	return impObj()->delX(rxclass.impObj());
}

PyRxObject PyRxClass::queryX(const PyRxClass& rxclass)
{
	if (!impObj() || !rxclass.impObj()) return PyRxObject();
	return impObj()->queryX(rxclass.impObj());
}

PyRxObject PyRxClass::queryX(const PyRxClass& rxclass) const
{
	if (!impObj() || !rxclass.impObj()) return PyRxObject();
	return impObj()->queryX(rxclass.impObj());
}

PyRxObject PyRxClass::create() {
	if (!impObj()) return PyRxObject();
	return impObj()->create();
}

PyRxClass PyRxClass::myParent() const
{
	if (!impObj()) return PyRxClass();
	return impObj()->myParent();
}



