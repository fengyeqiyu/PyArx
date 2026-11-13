#pragma once
#include "stdafx.h"
#include "PyDbObject.h"

class PyDbTableStyle : public PyDbObject {
public:
	PyWrapper_H(DbTableStyle)
	PyDbTableStyle(AcDbTableStyle* ptr) : PyDbObject(ptr) {}

	AcString getName() const;
	bool setName(const AcString& name);
	// isRenamable() const is not exported by the import library; left as skeleton.
	AcString description() const;
	bool setDescription(const AcString& desc);
	int bitFlags() const;
	bool setBitFlags(int flags);
	int flowDirection() const;
	bool setFlowDirection(int flow);
	double textHeight() const;
	bool setTextHeight(double h);
	// 文字样式 ObjectId（rowType 默认 kDataRow；setter rowTypes 默认 kAllRows）
	uint64_t textStyle(int rowType = AcDb::kDataRow) const;
	bool setTextStyle(uint64_t id, int rowTypes = AcDb::kAllRows);
	// 单元格对齐（AcDb::CellAlignment 枚举以 int 暴露）
	int alignment(int rowType = AcDb::kDataRow) const;
	bool setAlignment(int align, int rowTypes = AcDb::kAllRows);
	int numCellStyles() const;
	bool isCellStyleInUse(const AcString& cellStyle) const;

	// ---- 颜色 RGB（AcCmColor 未单独封装，按 (r,g,b) 出入参包装）----
	std::tuple<int,int,int> color(int rowType = AcDb::kDataRow) const;
	bool setColor(int r, int g, int b, int rowTypes = AcDb::kAllRows);
	std::tuple<int,int,int> backgroundColor(int rowType = AcDb::kDataRow) const;
	bool setBackgroundColor(int r, int g, int b, int rowTypes = AcDb::kAllRows);
	bool isBackgroundColorNone(int rowType = AcDb::kDataRow) const;
	bool setBackgroundColorNone(bool bValue, int rowTypes = AcDb::kAllRows);
	// ---- 数据类型 / 格式（AcValue::DataType/UnitType 以 int 暴露）----
	std::tuple<int,int> getDataType() const;
	bool setDataType(int dataType, int unitType);
	AcString format() const;
	bool setFormat(const AcString& fmt);
	// ---- 网格线属性（GridLineType/LineWeight/Visibility 以 int 暴露）----
	int gridLineWeight(int gridLineType, int rowType = AcDb::kDataRow) const;
	bool setGridLineWeight(int lineWeight, int gridLineTypes = AcDb::kAllGridLines, int rowTypes = AcDb::kAllRows);
	std::tuple<int,int,int> gridColor(int gridLineType, int rowType = AcDb::kDataRow) const;
	bool setGridColor(int r, int g, int b, int gridLineTypes = AcDb::kAllGridLines, int rowTypes = AcDb::kAllRows);
	int gridVisibility(int gridLineType, int rowType = AcDb::kDataRow) const;
	bool setGridVisibility(int visible, int gridLineTypes = AcDb::kAllGridLines, int rowTypes = AcDb::kAllRows);
	// ---- 边距 / 标题与表头抑制 ----
	double horzCellMargin() const;
	bool setHorzCellMargin(double m);
	double vertCellMargin() const;
	bool setVertCellMargin(double m);
	bool isTitleSuppressed() const;
	bool isHeaderSuppressed() const;
	bool suppressTitleRow(bool b);
	bool suppressHeaderRow(bool b);
	double margin(int cellMargin, const AcString& cellStyle) const;
	bool setMargin(int cellMargin, double fMargin, const AcString& cellStyle);

	// ---- Skeleton (not bound; cell-style name array (AcStringArray), cell-class,
	// rotation, create/rename/delete/copyCellStyle, postTableStyleToDb(out ObjectId),
	// getTemplate/setTemplate(AcDbTableTemplate), grid linetype/style/double-line-spacing,
	// subGetClassID...) ----
};

void bind_PyDbTableStyle(pym& m);
