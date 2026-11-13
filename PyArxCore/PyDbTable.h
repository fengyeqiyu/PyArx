#pragma once
#include "stdafx.h"
#include "PyDbBlockReference.h"

class PyDbTable : public PyDbBlockReference {
	PyWrapper_H(DbTable)
public:
	PyDbTable(AcDbTable* p) : PyDbBlockReference(p) {}
	PyDbTable() : PyDbTable(new AcDbTable()) {}
	Adesk::UInt32 numRows() const;
	Adesk::UInt32 numColumns() const;
	bool setSize(int nRows, int nCols);
	double width() const;
	bool setWidth(double);
	double height() const;
	bool setHeight(double);
	double columnWidth(int col) const;
	bool setColumnWidth(int col, double width);
	double rowHeight(int row) const;
	bool setRowHeight(int row, double height);
	AcString textString(int row, int col) const;
	bool setTextString(int row, int col, const AcString& text);
	double textHeight(int row, int col) const;
	bool setTextHeight(int row, int col, double h);
	bool insertColumns(int col, double width, int nCols = 1);
	bool deleteColumns(int col, int nCols = 1);
	bool insertRows(int row, double height, int nRows = 1);
	bool deleteRows(int row, int nRows = 1);
	bool mergeCells(int minRow, int maxRow, int minCol, int maxCol);
	bool unmergeCells(int minRow, int maxRow, int minCol, int maxCol);
	bool isMergedCell(int row, int col) const;
	bool isRegenerateTableSuppressed() const;
	void suppressRegenerateTable(bool);
	bool setPosition(const AcGePoint3d&);
	int flowDirection() const;
	bool setFlowDirection(int);
	bool generateLayout();
	bool recomputeTableBlock(bool forceUpdate = true);
	uint64_t tableStyleId() const;
	bool setNumRows(int nRows);
	bool setNumColumns(int nCols);
	bool setTableStyle(uint64_t id);
	AcGeVector3d direction() const;
	bool setDirection(const AcGeVector3d&);
	double rotation() const;
	bool setRotation(double);
	AcString toString() const;
};

void bind_PyDbTable(pym& m);
