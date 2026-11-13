#include "stdafx.h"
#include "PyRxClass.h"
#include "PyRxObject.h"
#include "PyGiSubEntityTraits.h"
#include "PyDbEntity.h"

//#include "PyGiMapper.h"
//#include "PyGiFill.h"


PyWrapper_Cpp(GiSubEntityTraits)
PyWrapper_Cpp(GiDrawableTraits)

void PyGiSubEntityTraits::setColor(const Adesk::UInt16 color) {
	impObj()->setColor(color);
}
void PyGiSubEntityTraits::setTrueColor(const AcCmEntityColor& color) {
	impObj()->setTrueColor(color);
}
void PyGiSubEntityTraits::setLayer(const AcDbObjectId layerId) {
	impObj()->setLayer(layerId);
}
void PyGiSubEntityTraits::setLineType(const AcDbObjectId linetypeId) {
	impObj()->setLineType(linetypeId);
}
void PyGiSubEntityTraits::setSelectionMarker(const Adesk::LongPtr markerId) {
	impObj()->setSelectionMarker(markerId);
}
void PyGiSubEntityTraits::setFillType(const AcGiFillType filltype) {
	impObj()->setFillType(filltype);
}
void PyGiSubEntityTraits::setLineWeight(const AcDb::LineWeight lw) {
	impObj()->setLineWeight(lw);
}
void PyGiSubEntityTraits::setLineWeight(const AcDb::LineWeight lw, const char lweight_index) {
	impObj()->setLineWeight(lw, lweight_index);
}

void PyGiSubEntityTraits::setLineTypeScale(double dScale) {
	impObj()->setLineTypeScale(dScale);
}
void PyGiSubEntityTraits::setThickness(double dThickness) {
	impObj()->setThickness(dThickness);
}
void PyGiSubEntityTraits::setVisualStyle(const AcDbObjectId visualStyleId) {
	impObj()->setVisualStyle(visualStyleId);
}
void PyGiSubEntityTraits::setPlotStyleName(AcDb::PlotStyleNameType pltType, const AcDbObjectId& objid) {
	impObj()->setPlotStyleName(pltType, objid);
}
void PyGiSubEntityTraits::setMaterial(const AcDbObjectId materialId) {
	impObj()->setMaterial(materialId);
}

//void PyGiSubEntityTraits::setMapper(const PyGiMapper& mapper) {
//	impObj()->setMapper(mapper.impObj());
//}
void PyGiSubEntityTraits::setSectionable(bool bSectionable) {
	impObj()->setSectionable(bSectionable);
}
Acad::ErrorStatus PyGiSubEntityTraits::setDrawFlags(Adesk::UInt32 flags) {
	return impObj()->setDrawFlags(flags);
}
Acad::ErrorStatus PyGiSubEntityTraits::setShadowFlags(AcGiSubEntityTraits::ShadowFlags flags) {
	return impObj()->setShadowFlags(flags);
}
void PyGiSubEntityTraits::setSelectionGeom(bool bSelectionflag) {
	impObj()->setSelectionGeom(bSelectionflag);
}
void PyGiSubEntityTraits::setTransparency(const AcCmTransparency& transparency) {
	impObj()->setTransparency(transparency);
}

//void PyGiSubEntityTraits::setFill(const PyGiFill& pFill) {
//	impObj()->setFill(pFill.impObj());
//}

// Return current settings.
//
Adesk::UInt16 PyGiSubEntityTraits::color(void) const {
	return impObj()->color();
}
AcCmEntityColor PyGiSubEntityTraits::trueColor(void) const {
	return impObj()->trueColor();
}
AcDbObjectId PyGiSubEntityTraits::layerId(void) const {
	return impObj()->layerId();
}
AcDbObjectId PyGiSubEntityTraits::lineTypeId(void) const {
	return impObj()->lineTypeId();
}
AcGiFillType PyGiSubEntityTraits::fillType(void) const {
	return impObj()->fillType();
}
AcDb::LineWeight PyGiSubEntityTraits::lineWeight(void) const {
	return impObj()->lineWeight();
} 
double PyGiSubEntityTraits::lineTypeScale(void) const {
	return impObj()->lineTypeScale();
}
double PyGiSubEntityTraits::thickness(void) const {
	return impObj()->thickness();
}
AcDbObjectId PyGiSubEntityTraits::visualStyle(void) const {
	return impObj()->visualStyle();
}

AcDbObjectId PyGiSubEntityTraits::materialId(void) const {
	return impObj()->materialId();
}
//const PyGiMapper& PyGiSubEntityTraits::mapper(void) const {
//	return impObj()->mapper();
//}
bool PyGiSubEntityTraits::sectionable(void) const {
	return impObj()->sectionable();
}
Adesk::UInt32 PyGiSubEntityTraits::drawFlags(void) const {
	return impObj()->drawFlags();
}
AcGiSubEntityTraits::ShadowFlags PyGiSubEntityTraits::shadowFlags(void) const {
	return impObj()->shadowFlags();
}
bool PyGiSubEntityTraits::selectionGeom(void) const {
	return impObj()->selectionGeom();
}
AcCmTransparency PyGiSubEntityTraits::transparency(void) const {
	return impObj()->transparency();
}
//const PyGiFill& PyGiSubEntityTraits::fill(void) const {
//	return impObj()->fill();
//}

void PyGiDrawableTraits::setupForEntity(PyDbEntity& pEntity) 
{
	impObj()->setupForEntity(pEntity.impObj());
}
