#pragma once
#include "PyRxObject.h"

#pragma pack (push, 8)
class PyGiDrawable;
class PyGiWorldGeometry;
class PyGiViewportGeometry;
class PyDbPolyline;
class PyGiSubEntityTraits;


class PyGiCommonDraw : public PyRxObject
{
	PyWrapper_H(GiCommonDraw)
public:
	PyGiCommonDraw(AcGiCommonDraw* ptr, bool autoDelete = false) :PyRxObject(ptr, autoDelete, false) {}
	virtual ~PyGiCommonDraw() override = default;
	PyGiSubEntityTraits subEntityTraits() const;
	AcGiRegenType regenType() const;
	Adesk::Boolean regenAbort() const;
	Adesk::Boolean isDragging() const;

};


class PyGiWorldDraw : public PyGiCommonDraw
{
	PyWrapper_H(GiWorldDraw)
public:
	PyGiWorldDraw(AcGiWorldDraw* ptr) :PyGiCommonDraw(ptr) {
	}
	virtual ~PyGiWorldDraw() override = default;
	PyGiWorldGeometry geometry() const;
};



class PyGiViewportDraw : public PyGiCommonDraw
{
	PyWrapper_H(GiViewportDraw)
public:
	PyGiViewportDraw(AcGiViewportDraw* ptr, bool autoDelete = false) :PyGiCommonDraw(ptr) {
	}
	virtual ~PyGiViewportDraw() override = default;

	PyGiViewportGeometry geometry() const;

};

//-----------------------------------------------------------------------------------------

class PyGiGeometry : public PyRxObject
{
	PyWrapper_H(GiGeometry)
public:
	PyGiGeometry(AcGiGeometry* ptr) :PyRxObject(ptr, false, false) {
	};
	virtual ~PyGiGeometry() override = default;

};

class PyGiWorldGeometry : public PyGiGeometry
{
	PyWrapper_H(GiWorldGeometry)
public:
	PyGiWorldGeometry(AcGiWorldGeometry* ptr) :PyGiGeometry(ptr) {
	}
	virtual ~PyGiWorldGeometry() override = default;


};


class PyGiViewportGeometry : public PyGiGeometry
{
	PyWrapper_H(GiViewportGeometry)
public:
	PyGiViewportGeometry(AcGiViewportGeometry* ptr) :PyGiGeometry(ptr) {}
	virtual ~PyGiViewportGeometry() override = default;
};
