//PyRxClass.cpp
#include "stdafx.h"
#include "PyRxObject.h"
#include "stdafx.h"
#include "PyRxClass.h"

PyWrapper_Cpp(RxClass)



bool PyRxClass::isDerivedFrom(const PyRxClass& otherClass) const
{
	return AcRxClass::isDerivedFrom(impObj()->isA(), otherClass.impObj());
}


int PyRxClass::proxyFlags() const
{
	return impObj()->proxyFlags();
}


PyRxObject PyRxClass::addX(PyRxClass& rxclass, PyRxObject& rxobj)
{
	return impObj()->addX(rxclass.impObj(), rxobj.impObj());
}

PyRxObject PyRxClass::getX(const PyRxClass& rxobj)
{
	return impObj()->getX(rxobj.impObj());
}

PyRxObject PyRxClass::delX(PyRxClass& rxclass)
{
	return impObj()->delX(rxclass.impObj());
}

PyRxObject PyRxClass::queryX(const PyRxClass& rxclass)
{
	return impObj()->queryX(rxclass.impObj());
}

PyRxObject PyRxClass::queryX(const PyRxClass& rxclass) const
{
	return impObj()->queryX(rxclass.impObj());
}

PyRxObject PyRxClass::create() {
	return impObj()->create();
}

PyRxClass PyRxClass::myParent() const
{
	return impObj()->myParent();
}



