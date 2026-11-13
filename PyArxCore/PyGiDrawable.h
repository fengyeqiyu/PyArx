#pragma once
#include "stdafx.h"
#include "PyRxObject.h"

#include "PyDbExtents.h"


class PyGiWorldDraw;
class PyGiViewportDraw;
class PyGiDrawableTraits;
class PyDbExtents3d;
typedef  AcGiDrawable::DrawableType DrawableType;

class PyGiDrawable : public PyRxObject {
	PyWrapper_H(GiDrawable);
	PyGiDrawable(AcGiDrawable* ptr, bool isDbObject = false, bool autoDelete = false) : PyRxObject(ptr, isDbObject, autoDelete) {}
	virtual ~PyGiDrawable() override = default;

	virtual Adesk::Boolean  isPersistent(void) const;
	PyDbExtents3d bounds() const;
	virtual AcDbObjectId    ObjectId(void) const;
	virtual bool            bounds(AcDbExtents& bounds) const;
	virtual AcGiDrawable::DrawableType drawableType(void) const;

	// ������ط���
	virtual bool            worldDraw(PyGiWorldDraw& wd);
	virtual Adesk::Boolean  subWorldDraw(PyGiWorldDraw& wd);

	virtual void            viewportDraw(PyGiViewportDraw& vpd);
	virtual void            subViewportDraw(PyGiViewportDraw& vd);

	virtual Adesk::UInt32   viewportDrawLogicalFlags(PyGiViewportDraw& vd);
	virtual Adesk::UInt32   subViewportDrawLogicalFlags(PyGiViewportDraw& svd);

	// ��������
	virtual unsigned int    setAttributes(PyGiDrawableTraits& traits);
	virtual Adesk::UInt32   subSetAttributes(PyGiDrawableTraits& traits);

	// �������
	virtual Adesk::Boolean  rolloverHit(Adesk::ULongPtr nSubentId, Adesk::ULongPtr nMouseFlags, Adesk::Boolean bReset);

};
