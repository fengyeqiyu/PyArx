#pragma once
#include "stdafx.h"
#include "PyDbObject.h"
#include "PyDbEntity.h"
#include "PyDbDatabase.h"
#include "PyDbSymbolTableRecord.h"

class PyDbLayerTableRecord : public PyDbSymbolTableRecord {
public:
	PyWrapper_H(DbLayerTableRecord)
	PyDbLayerTableRecord(AcDbLayerTableRecord* ptr) : PyDbSymbolTableRecord(ptr) {}
	bool isFrozen() const;
	bool setIsFrozen(bool frozen);
	bool isOff() const;
	void setIsOff(bool off);
	bool VPDFLT() const;
	void setVPDFLT(bool frozen);
	bool isLocked() const;
	void setIsLocked(bool locked);
	uint64_t linetypeObjectId() const;
	bool setLinetypeObjectId(uint64_t id);
	uint64_t materialId() const;
	bool setMaterialId(uint64_t id);
	// 线宽（AcDb::LineWeight 枚举以 int 暴露；颜色/透明度需 AcCmColor，按约定延迟）
	int lineWeight() const;
	bool setLineWeight(int lw);
	bool isPlottable() const;
	bool setIsPlottable(bool plot);
	AcString plotStyleName() const;
	uint64_t plotStyleNameId() const;
	bool setPlotStyleName(const AcString& newName);
	bool setPlotStyleNameById(uint64_t newId);
	bool isInUse() const;
	AcString description() const;
	bool setDescription(const AcString& description);
	bool isHidden() const;
	bool setIsHidden(bool on);
	static bool isHidden(uint64_t objectId);
	bool removeColorOverride(uint64_t viewportId);
	bool removeLinetypeOverride(uint64_t viewportId);
	bool removeLineWeightOverride(uint64_t viewportId);
	bool removePlotStyleOverride(uint64_t viewportId);
	bool removeTransparencyOverride(uint64_t viewportId);
	bool removeViewportOverrides(uint64_t viewportId);
	bool removeAllOverrides();
	bool hasOverrides(uint64_t viewportId) const;
	bool hasAnyOverrides() const;
	bool isReconciled() const;
	bool setIsReconciled(bool bReconcile = true);
	static bool isReconciled(uint64_t objectId);

	// ---- 颜色 / 透明度（AcCmColor/AcCmEntityColor/AcCmTransparency 未单独封装，安全透出为整数）----
	// color() -> (colorIndex, r, g, b)；colorIndex 为 ACI（256=ByLayer,0=ByBlock），RGB 仅在真彩色时有效
	std::tuple<int, int, int, int> color() const;
	// setColor(r,g,b) 直接以真彩色写入
	void setColor(int r, int g, int b);
	// entityColor() -> (colorIndex, r, g, b)
	std::tuple<int, int, int, int> entityColor() const;
	// transparency() -> int：0=ByLayer，1=ByBlock，否则为 alpha 0..255
	int transparency() const;
	// setTransparency(int)：0=ByLayer，1=ByBlock，否则为 alpha 0..255
	bool setTransparency(int trans);

	// ---- Skeleton (not bound; export/type uncertain, needs review) ----
	// color() const -> AcCmColor / setColor(const AcCmColor&)
	// entityColor() const -> AcCmEntityColor
	// transparency() const -> AcCmTransparency / setTransparency(const AcCmTransparency&)
	// lineWeight() const -> AcDb::LineWeight / setLineWeight(AcDb::LineWeight)
	// per-viewport color/linetype/lineWeight/plotStyleName/transparency overrides
	//   (viewportId + out bool& isOverride)
};

void bind_PyDbLayerTableRecord(pym& m);
