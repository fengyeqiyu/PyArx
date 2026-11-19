#pragma once
#include "stdafx.h"
#include "PyDbObject.h"
#include "PyColor.h"

// Base wrapper for AcDbEntity (pointer ctor only; abstract root of all entities).
// Common entity capability is bound here once; derived entities inherit it and
// must not rebind these methods.
class PyDbEntity : public PyDbObject {
	PyWrapper_H(DbEntity)
public:
	PyDbEntity(AcDbEntity* p) : PyDbObject(p) {}

	unsigned short colorIndex() const;
	bool setColorIndex(unsigned short colorIndex, bool doSubents = true);

	AcString layer() const;
	uint64_t layerId() const;
	bool setLayer(const AcString& name, bool doSubents = true);
	bool setLayerId(uint64_t layerId, bool doSubents = true);

	AcString linetype() const;
	uint64_t linetypeId() const;
	bool setLinetype(const AcString& name, bool doSubents = true);

	int lineWeight() const;
	bool setLineWeight(int newVal, bool doSubents = true);

	int visibility() const;
	bool setVisibility(int newVal, bool doSubents = true);

	double linetypeScale() const;
	bool setLinetypeScale(double newval, bool doSubents = true);

	py::object getGeomExtents() const;
	bool transformBy(const AcGeMatrix3d& xform);
	py::list intersectWith(const PyDbEntity& other, int intType) const;

	// --- 颜色 / 透明度（AcCm* 未单独绑定，提取为基本类型）---
	PyColor color() const;
	bool setColorByRgb(int r, int g, int b, bool doSubents = true);   // 真彩色 RGB 设置（分量夹紧 0-255）
	std::tuple<int,int,int> entityColor() const;          // 有效颜色 RGB
	std::tuple<int,int,int> colorRGB() const;             // 存储真彩色 RGB
	int  transparency() const;                            // alpha 0=全透明 255=不透明
	bool setTransparency(int alpha, bool doSubents = true);

	// --- 包围盒 ---
	std::tuple<AcGePoint3d,AcGePoint3d> bounds() const;  // (minPoint, maxPoint)

	// --- 平面性 ---
	std::tuple<AcGePlane,int> getPlane() const;          // (plane, planarity枚举)
	bool isPlanar() const;
	bool isContentSnappable() const;

	// --- 材质 ---
	AcString material() const;
	uint64_t materialId() const;
	bool setMaterial(const AcString& name, bool doSubents = true);

	// --- 数据库默认值 / 属性复制 ---
	void setDatabaseDefaults();
	bool setPropertiesFrom(const PyDbEntity& src, bool doSubents = true);

	// --- 阴影 ---
	bool castShadows() const;
	bool setCastShadows(bool newVal);
	bool receiveShadows() const;
	bool setReceiveShadows(bool newVal);

	// --- 其它显示 / 图形状态 ---
	int  collisionType() const;
	void recordGraphicsModified(bool setModified = true);
	uint64_t visualStyleId() const;
	bool setVisualStyle(uint64_t visualStyleId, bool doSubents = true);
	uint64_t blockId() const;
	AcGeMatrix3d getEcs() const;

	// --- 打印样式 ---
	AcString plotStyleName() const;
	bool setPlotStyleName(const AcString& name);

	// --- 其它 ---
	void list() const;   // 等价 CAD LIST 命令
	void draw() const;   // 绘制到当前视图
	py::tuple getGripPoints() const;  // 返回 (gripPoints, osnapModes, geomIds)
	void moveGripPointsAt(const std::vector<int>& indices, const AcGeVector3d& offset);

	// --- 高亮 ---
	bool pushHighlight(int style = 0);
	bool popHighlight();

	// --- 拉伸点 ---
	py::tuple getStretchPoints() const;
	void moveStretchPointsAt(const std::vector<int>& indices, const AcGeVector3d& offset);
};

void bind_PyDbEntity(pym& m);
