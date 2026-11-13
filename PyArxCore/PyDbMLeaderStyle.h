#pragma once
#include "stdafx.h"
#include <tuple>
#include "PyDbObject.h"
#include <dbmleaderstyle.h>
#include "PyColor.h"
#include "PyDbDatabase.h"

// ===========================================================================
// PyDbMLeaderStyle —— AcDbMLeaderStyle（多重引线样式）数据库对象包装
//
// AcDbMLeaderStyle 直接继承 AcDbObject（非 Entity / 非 SymbolTableRecord）。
// 本 wrapper 以 PyDbObject 为基类，提供：
//   - 名称 / 描述 / 可重命名
//   - 内容类型（多行文字 / 块）
//   - 引线：最大段点数、首/二段角度约束、引线线型、落地/狗腿
//   - 箭头：符号 ID、箭头大小
//   - 文字：文字样式、对齐、始终左对齐、高度、文字颜色、文字框、对齐间距、默认文字
//   - 块：块 ID、块颜色、块比例/旋转
//   - 比例 / 标注性 / 断开尺寸 / 文字附着方向 / 延伸到文字
//   - 入库：postMLeaderStyleToDb
//
// 错误处理遵循项目约定：SDK 调用失败不抛异常，acutPrintf 打印到 CAD 命令行；
// setter 返回 bool（成功 true / 失败 false）；查询失败返回默认值。
// ===========================================================================
class PyDbMLeaderStyle : public PyDbObject {
	PyWrapper_H(DbMLeaderStyle)
public:
	PyDbMLeaderStyle(AcDbMLeaderStyle* p) : PyDbObject(p) {}
	PyDbMLeaderStyle() : PyDbMLeaderStyle(new AcDbMLeaderStyle()) {}

	// ---- 名称 / 描述 ----
	AcString getName() const;
	bool setName(const AcString&);
	AcString description() const;
	bool setDescription(const AcString&);

	// ---- 内容类型 ----
	int contentType() const;
	bool setContentType(int);

	// ---- 引线 ----
	int maxLeaderSegmentsPoints() const;
	bool setMaxLeaderSegmentsPoints(int);
	int firstSegmentAngleConstraint() const;
	bool setFirstSegmentAngleConstraint(int);
	int secondSegmentAngleConstraint() const;
	bool setSecondSegmentAngleConstraint(int);
	uint64_t leaderLineTypeId() const;
	bool setLeaderLineTypeId(uint64_t);
	bool enableLanding() const;
	bool setEnableLanding(bool);
	double landingGap() const;
	bool setLandingGap(double);
	bool enableDogleg() const;
	bool setEnableDogleg(bool);
	double doglegLength() const;
	bool setDoglegLength(double);

	// ---- 箭头 ----
	uint64_t arrowSymbolId() const;
	bool setArrowSymbolId(uint64_t);
	double arrowSize() const;
	bool setArrowSize(double);

	// ---- 文字 ----
	uint64_t textStyleId() const;
	bool setTextStyleId(uint64_t);
	int textAlignmentType() const;
	bool setTextAlignmentType(int);
	bool textAlignAlwaysLeft() const;
	bool setTextAlignAlwaysLeft(bool);
	double textHeight() const;
	bool setTextHeight(double);
	PyColor textColor() const;
	bool setTextColor(const PyColor&);
	bool enableFrameText() const;
	bool setEnableFrameText(bool);
	double alignSpace() const;
	bool setAlignSpace(double);
	bool setDefaultMText(uint64_t mtextId);
	uint64_t defaultMText() const;

	// ---- 块 ----
	uint64_t blockId() const;
	bool setBlockId(uint64_t);
	PyColor blockColor() const;
	bool setBlockColor(const PyColor&);
	bool enableBlockScale() const;
	bool setEnableBlockScale(bool);
	double blockRotation() const;
	bool setBlockRotation(double);
	bool enableBlockRotation() const;
	bool setEnableBlockRotation(bool);

	// ---- 比例 / 标注性 / 其他 ----
	double scale() const;
	bool setScale(double);
	bool overwritePropChanged() const;
	bool annotative() const;
	bool setAnnotative(bool);
	double breakSize() const;
	bool setBreakSize(double);
	int textAttachmentDirection() const;
	bool setTextAttachmentDirection(int);
	bool extendLeaderToText() const;
	bool setExtendLeaderToText(bool);

	// ---- 入库 ----
	std::tuple<bool, uint64_t> postMLeaderStyleToDb(PyDbDatabase& db, const AcString& styleName);

	AcString toString() const;
};

void bind_PyDbMLeaderStyle(pym& m);
