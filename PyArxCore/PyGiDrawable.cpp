
#include "stdafx.h"
#include "PyGiDrawable.h"
#include "PyRxObject.h"
#include "PyRxClass.h"
//#include "PyGiCommonDraw.h"
#include "PyGiSubEntityTraits.h"
#include "PyGiWorldGeometry.h"
#include "PyDbSymbolTable.h"
#include "PyDbDatabase.h"

#pragma warning(disable: 4197)

PyWrapper_Cpp(GiDrawable)
//PyWrapper_Cpp(GiDrawableOverrule)
//PyWrapper_Cpp(GiDrawableReactor)



Adesk::Boolean PyGiDrawable::isPersistent(void) const {
	if (!impObj()) return Adesk::kFalse;
	return impObj()->isPersistent();
}
AcDbObjectId    PyGiDrawable::ObjectId(void) const {
	if (!impObj()) return {};
	return impObj()->id();
}
PyDbExtents3d     PyGiDrawable::bounds() const {
	if (!impObj()) return {};
	AcDbExtents bounds;
	auto res = impObj()->bounds(bounds);
	if (res == true) {
		return PyDbExtents3d(bounds, false);
	}
	return PyDbExtents3d();
}

bool PyGiDrawable::bounds(AcDbExtents& bounds) const {
	if (!impObj()) return false;
	return impObj()->bounds(bounds);
}
AcGiDrawable::DrawableType  PyGiDrawable::drawableType(void) const {
	return AcGiDrawable::kGeometry;
}

bool PyGiDrawable::worldDraw(PyGiWorldDraw& wd)
{
	return impObj()->worldDraw(wd.impObj());
}

Adesk::Boolean PyGiDrawable::subWorldDraw(PyGiWorldDraw& wd)
{
	return impObj()->subWorldDraw(wd.impObj());
}

void PyGiDrawable::viewportDraw(PyGiViewportDraw& vpd)
{
	return impObj()->viewportDraw(vpd.impObj());
}

void PyGiDrawable::subViewportDraw(PyGiViewportDraw& vd)
{
	return impObj()->subViewportDraw(vd.impObj());
}

Adesk::UInt32 PyGiDrawable::viewportDrawLogicalFlags(PyGiViewportDraw& vd)
{
	return impObj()->viewportDrawLogicalFlags(vd.impObj());
}

Adesk::UInt32 PyGiDrawable::subViewportDrawLogicalFlags(PyGiViewportDraw& svd)
{
	return impObj()->subViewportDrawLogicalFlags(svd.impObj());
}

unsigned int PyGiDrawable::setAttributes(PyGiDrawableTraits& traits)
{
	return impObj()->setAttributes(traits.impObj());
}

Adesk::UInt32 PyGiDrawable::subSetAttributes(PyGiDrawableTraits& traits)
{
	return impObj()->subSetAttributes(traits.impObj());
}

Adesk::Boolean  PyGiDrawable::rolloverHit(Adesk::ULongPtr nSubentId, Adesk::ULongPtr nMouseFlags, Adesk::Boolean bReset) {
	if (!impObj()) return Adesk::kFalse;
	return impObj()->rolloverHit(nSubentId, nMouseFlags, bReset);
}