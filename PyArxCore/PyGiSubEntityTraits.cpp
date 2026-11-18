#include "stdafx.h"
#include "PyRxClass.h"
#include "PyRxObject.h"
#include "PyGiSubEntityTraits.h"
#include "PyGiFill.h"
#include "PyDbEntity.h"


PyWrapper_Cpp(GiSubEntityTraits)
PyWrapper_Cpp(GiDrawableTraits)

void PyGiSubEntityTraits::setColor(const Adesk::UInt16 color) {
	if (!impObj()) return;
	impObj()->setColor(color);
}
void PyGiSubEntityTraits::setTrueColor(const AcCmEntityColor& color) {
	if (!impObj()) return;
	impObj()->setTrueColor(color);
}
void PyGiSubEntityTraits::setLayer(const AcDbObjectId layerId) {
	if (!impObj()) return;
	impObj()->setLayer(layerId);
}
void PyGiSubEntityTraits::setLineType(const AcDbObjectId linetypeId) {
	if (!impObj()) return;
	impObj()->setLineType(linetypeId);
}
void PyGiSubEntityTraits::setSelectionMarker(const Adesk::LongPtr markerId) {
	if (!impObj()) return;
	impObj()->setSelectionMarker(markerId);
}
void PyGiSubEntityTraits::setFillType(const AcGiFillType filltype) {
	if (!impObj()) return;
	impObj()->setFillType(filltype);
}
void PyGiSubEntityTraits::setLineWeight(const AcDb::LineWeight lw) {
	if (!impObj()) return;
	impObj()->setLineWeight(lw);
}
void PyGiSubEntityTraits::setLineWeight(const AcDb::LineWeight lw, const char lweight_index) {
	if (!impObj()) return;
	impObj()->setLineWeight(lw, lweight_index);
}

void PyGiSubEntityTraits::setLineTypeScale(double dScale) {
	if (!impObj()) return;
	impObj()->setLineTypeScale(dScale);
}
void PyGiSubEntityTraits::setThickness(double dThickness) {
	if (!impObj()) return;
	impObj()->setThickness(dThickness);
}
void PyGiSubEntityTraits::setVisualStyle(const AcDbObjectId visualStyleId) {
	if (!impObj()) return;
	impObj()->setVisualStyle(visualStyleId);
}
void PyGiSubEntityTraits::setPlotStyleName(AcDb::PlotStyleNameType pltType, const AcDbObjectId& objid) {
	if (!impObj()) return;
	impObj()->setPlotStyleName(pltType, objid);
}
void PyGiSubEntityTraits::setMaterial(const AcDbObjectId materialId) {
	if (!impObj()) return;
	impObj()->setMaterial(materialId);
}
void PyGiSubEntityTraits::setMapper(const AcGiMapper& mapper) {
	if (!impObj()) return;
	impObj()->setMapper(&mapper);
}
void PyGiSubEntityTraits::setSectionable(bool bSectionable) {
	if (!impObj()) return;
	impObj()->setSectionable(bSectionable);
}
Acad::ErrorStatus PyGiSubEntityTraits::setDrawFlags(Adesk::UInt32 flags) {
	if (!impObj()) return Acad::eNullObjectPointer;
	return impObj()->setDrawFlags(flags);
}
Acad::ErrorStatus PyGiSubEntityTraits::setShadowFlags(AcGiSubEntityTraits::ShadowFlags flags) {
	if (!impObj()) return Acad::eNullObjectPointer;
	return impObj()->setShadowFlags(flags);
}
void PyGiSubEntityTraits::setSelectionGeom(bool bSelectionflag) {
	if (!impObj()) return;
	impObj()->setSelectionGeom(bSelectionflag);
}
void PyGiSubEntityTraits::setTransparency(const AcCmTransparency& transparency) {
	if (!impObj()) return;
	impObj()->setTransparency(transparency);
}
void PyGiSubEntityTraits::setFill(const PyGiFill& pFill) {
	if (!impObj()) return;
	impObj()->setFill(pFill.impObj());
}

// Return current settings.
//
Adesk::UInt16 PyGiSubEntityTraits::color(void) const {
	if (!impObj()) return 0;
	return impObj()->color();
}
AcCmEntityColor PyGiSubEntityTraits::trueColor(void) const {
	if (!impObj()) return {};
	return impObj()->trueColor();
}
AcDbObjectId PyGiSubEntityTraits::layerId(void) const {
	if (!impObj()) return {};
	return impObj()->layerId();
}
AcDbObjectId PyGiSubEntityTraits::lineTypeId(void) const {
	if (!impObj()) return {};
	return impObj()->lineTypeId();
}
AcGiFillType PyGiSubEntityTraits::fillType(void) const {
	if (!impObj()) return {};
	return impObj()->fillType();
}
AcDb::LineWeight PyGiSubEntityTraits::lineWeight(void) const {
	if (!impObj()) return {};
	return impObj()->lineWeight();
}
double PyGiSubEntityTraits::lineTypeScale(void) const {
	if (!impObj()) return 0.0;
	return impObj()->lineTypeScale();
}
double PyGiSubEntityTraits::thickness(void) const {
	if (!impObj()) return 0.0;
	return impObj()->thickness();
}
AcDbObjectId PyGiSubEntityTraits::visualStyle(void) const {
	if (!impObj()) return {};
	return impObj()->visualStyle();
}
AcDb::PlotStyleNameType PyGiSubEntityTraits::getPlotStyleNameId(AcDbObjectId& idResult) const {
	if (!impObj()) return {};
	return impObj()->getPlotStyleNameId(idResult);
}
AcDbObjectId PyGiSubEntityTraits::materialId(void) const {
	if (!impObj()) return {};
	return impObj()->materialId();
}
std::optional<AcGiMapper> PyGiSubEntityTraits::mapper(void) const {
	if (!impObj()) return {};
	const AcGiMapper* p = impObj()->mapper();
	if (!p) return std::nullopt;
	return *p;
}
bool PyGiSubEntityTraits::sectionable(void) const {
	if (!impObj()) return false;
	return impObj()->sectionable();
}
Adesk::UInt32 PyGiSubEntityTraits::drawFlags(void) const {
	if (!impObj()) return 0;
	return impObj()->drawFlags();
}
AcGiSubEntityTraits::ShadowFlags PyGiSubEntityTraits::shadowFlags(void) const {
	if (!impObj()) return {};
	return impObj()->shadowFlags();
}
bool PyGiSubEntityTraits::selectionGeom(void) const {
	if (!impObj()) return false;
	return impObj()->selectionGeom();
}
AcCmTransparency PyGiSubEntityTraits::transparency(void) const {
	if (!impObj()) return {};
	return impObj()->transparency();
}

void PyGiSubEntityTraits::pushMarkerOverride(bool flag, const Adesk::LongPtr markerId) {
	if (!impObj()) return;
	impObj()->pushMarkerOverride(flag, markerId);
}
void PyGiSubEntityTraits::popMarkerOverride(void) {
	if (!impObj()) return;
	impObj()->popMarkerOverride();
}
void PyGiSubEntityTraits::clearMarkerOverride(void) {
	if (!impObj()) return;
	impObj()->clearMarkerOverride();
}

void PyGiDrawableTraits::setupForEntity(PyDbEntity& pEntity)
{
	impObj()->setupForEntity(pEntity.impObj());
}

void PyGiDrawableTraits::addLight(const AcDbObjectId& lightId) {
	static_cast<AcGiDrawableTraits*>(impObj())->addLight(lightId);
}

void PyGiDrawableTraits::setLinePattern(int linePattern) {
	static_cast<AcGiDrawableTraits*>(impObj())->setLinePattern(static_cast<AcGiLineType>(linePattern));
}

int PyGiDrawableTraits::linePattern() const {
	return static_cast<int>(static_cast<AcGiDrawableTraits*>(impObj())->linePattern());
}

Acad::ErrorStatus PyGiDrawableTraits::setSelectionFlags(AcGiSubEntityTraits::SelectionFlags flags) {
	return static_cast<AcGiDrawableTraits*>(impObj())->setSelectionFlags(flags);
}

AcGiSubEntityTraits::SelectionFlags PyGiDrawableTraits::selectionFlags() const {
	return static_cast<AcGiDrawableTraits*>(impObj())->selectionFlags();
}

Acad::ErrorStatus PyGiDrawableTraits::setHighlightProperty(AcGiDrawableTraits::HighlightProperty property, AcGiVariant& value) {
	return static_cast<AcGiDrawableTraits*>(impObj())->setHighlightProperty(property, value);
}

AcGiVariant PyGiDrawableTraits::highlightProperty(AcGiDrawableTraits::HighlightProperty property) const {
	return static_cast<AcGiDrawableTraits*>(impObj())->highlightProperty(property);
}
