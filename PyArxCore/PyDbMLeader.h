#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include <dbmleader.h>

class PyDbMLeader : public PyDbEntity {
	PyWrapper_H(DbMLeader)
public:
	PyDbMLeader(AcDbMLeader* p) : PyDbEntity(p) {}
	PyDbMLeader() : PyDbMLeader(new AcDbMLeader()) {}
	int numLeaderLines() const;
	double scale() const;
	bool setScale(double);
	int contentType() const;
	bool setContentType(int);
	bool enableAnnotationScale() const;
	bool setEnableAnnotationScale(bool);
	bool extendLeaderToText() const;
	bool setExtendLeaderToText(bool);
	int textAttachmentDirection() const;
	bool setTextAttachmentDirection(int);
	int numLeaders() const;
	AcGeVector3d normal() const;
	bool hasContent() const;
	double landingGap() const;
	bool setLandingGap(double);
	bool enableLanding() const;
	bool setEnableLanding(bool);
	bool enableDogleg() const;
	bool setEnableDogleg(bool);
	double arrowSize() const;
	bool setArrowSize(double);
	double textHeight() const;
	bool setTextHeight(double);
	double blockRotation() const;
	bool setBlockRotation(double);
	bool updateLeaderLinePosition();
	// ---- 构建族（T0 补齐 2026-10-21）：引线 / 引线直线 / 顶点 / 内容 / 样式 ----
	bool addLeader(int& leaderIndex);
	bool removeLeader(int leaderIndex);
	bool addLeaderLine(int leaderIndex, int& leaderLineIndex);
	bool addLeaderLineAtPoint(const AcGePoint3d& point, int& leaderLineIndex);
	bool removeLeaderLine(int leaderLineIndex);
	bool addFirstVertex(int leaderLineIndex, const AcGePoint3d& point);
	bool addLastVertex(int leaderLineIndex, const AcGePoint3d& point);
	bool numVertices(int leaderLineIndex, int& num) const;
	bool setVertex(int leaderLineIndex, int index, const AcGePoint3d& point);
	bool getVertex(int leaderLineIndex, int index, AcGePoint3d& point) const;
	bool setMText(const AcDbMText* pMText);
	bool setBlockContentId(AcDbObjectId blockId);
	AcDbObjectId blockContentId() const;
	bool setMLeaderStyle(AcDbObjectId newStyleId);
	AcDbObjectId MLeaderStyle() const;
	bool setTextLocation(const AcGePoint3d& location);
	bool getTextLocation(AcGePoint3d& location) const;
	bool postMLeaderToDb(AcDbDatabase* pDb);
	AcString toString() const;
};

void bind_PyDbMLeader(pym& m);
