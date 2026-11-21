#pragma once
#include "stdafx.h"
#include "PyRxObject.h"
#include "PyRxClass.h"


class PyDbEntity;
class PyGiFill;
class PyGiMapper;
//class PyGiMapper;
//class PyGiFill;
class PyGiSubEntityTraits : public PyRxObject
{
public:
	PyWrapper_H(GiSubEntityTraits)
public:
	PyGiSubEntityTraits(AcGiSubEntityTraits* ptr, bool autoDelete = false) : PyRxObject(ptr, autoDelete, false) {}
	virtual ~PyGiSubEntityTraits() override = default;
	void setColor(const Adesk::UInt16 color);
	void setTrueColor(const AcCmEntityColor& color);
	void setLayer(const AcDbObjectId layerId);
	void setLineType(const AcDbObjectId linetypeId);
	void setSelectionMarker(const Adesk::LongPtr markerId);
	void setFillType(const AcGiFillType fillType);
	void setLineWeight(const AcDb::LineWeight lw);
	void setLineWeight(const AcDb::LineWeight lw, const char lweight_index);
	void setLineTypeScale(double dScale = 1.0);
	void setThickness(double dThickness);
	void setVisualStyle(const AcDbObjectId visualStyleId);
	void setPlotStyleName(AcDb::PlotStyleNameType pltType, const AcDbObjectId& objid = AcDbObjectId::kNull);
	void setMaterial(const AcDbObjectId materialId);

	void setSectionable(bool bSectionable);
	Acad::ErrorStatus setDrawFlags(Adesk::UInt32 flags);
	Acad::ErrorStatus setShadowFlags(AcGiSubEntityTraits::ShadowFlags flags);
	void setSelectionGeom(bool bSelectionflag);
	void setTransparency(const AcCmTransparency& transparency);
	// [ADD] SDK: setFill / setMapper —— 填充（图案/渐变）与材质映射
	void setFill(const PyGiFill& pFill);
	void setMapper(const AcGiMapper& mapper);

	// Return current settings.
	//
	Adesk::UInt16 color(void) const;
	AcCmEntityColor trueColor(void) const;
	AcDbObjectId layerId(void) const;
	AcDbObjectId lineTypeId(void) const;
	AcGiFillType fillType(void) const;
	AcDb::LineWeight lineWeight(void) const;
	double lineTypeScale(void) const;
	double thickness(void) const;
	AcDbObjectId visualStyle(void) const;
	// [FIX] 原来写死返回 kPlotStyleNameByLayer，现直通 SDK（出参 id 一并返回）
	AcDb::PlotStyleNameType getPlotStyleNameId(AcDbObjectId& idResult) const;
	AcDbObjectId materialId(void) const;
	std::optional<AcGiMapper> mapper(void) const;

	bool sectionable(void) const;
	Adesk::UInt32 drawFlags(void) const;
	AcGiSubEntityTraits::ShadowFlags shadowFlags(void) const;
	bool selectionGeom(void) const;
	AcCmTransparency transparency(void) const;

	// [FIX] 原来三个方法都是空实现，现直通 SDK
	void pushMarkerOverride(bool flag, const Adesk::LongPtr markerId);
	void popMarkerOverride(void);
	void clearMarkerOverride(void);
};

class PyGiDrawableTraits : public PyGiSubEntityTraits
{
	PyWrapper_H(GiDrawableTraits)
public:
	PyGiDrawableTraits(AcGiDrawableTraits* ptr, bool autoDelete = false)
		:PyGiSubEntityTraits(ptr, autoDelete) {
	}

	~PyGiDrawableTraits() override = default;
	void setupForEntity(PyDbEntity& pEntity);
	// [ADD] SDK: addLight / setLinePattern / setSelectionFlags / setHighlightProperty
	void addLight(const AcDbObjectId& lightId);
	void setLinePattern(int linePattern);
	int linePattern() const;
	Acad::ErrorStatus setSelectionFlags(AcGiSubEntityTraits::SelectionFlags flags);
	AcGiSubEntityTraits::SelectionFlags selectionFlags() const;
	Acad::ErrorStatus setHighlightProperty(AcGiDrawableTraits::HighlightProperty property, AcGiVariant& value);
	AcGiVariant highlightProperty(AcGiDrawableTraits::HighlightProperty property) const;
};
