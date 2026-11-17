#pragma once
// ===========================================================================
// PyGiPolyline.h —— AcGiPolyline 值类封装（SDK: acgi.h）。
//
// AcGiGeometry::polyline(const AcGiPolyline&) / polyPolyline 的入参类型：
// 带 bulge / 变宽 / 闭合 / 升程 / 厚度 的多段折线参数对象。因为是非
// AcRxObject 派生的参数类（AcGiParameter），这里用独立包装类持有 new 出来的
// 对象（autoDelete 语义在析构里显式做），不挂 PyRxObject 继承链。
//
// 注意：AcGiEdgeData / AcGiFaceData / AcGiVertexData 刻意不绑定 —— SDK 的
// set* 接口吃裸指针数组、但不接收长度，Python list 的生命周期无法保证，
// 绑定出来只会造成悬垂指针。mesh/shell 只传顶点数组（其余参数默认 NULL）。
// ===========================================================================
#include "stdafx.h"

class PyGiPolyline {
public:
	PyGiPolyline();
	PyGiPolyline(const std::vector<AcGePoint3d>& pts,
		const std::optional<AcGeVector3d>& normal,
		long long baseSubEntMarker);
	~PyGiPolyline();

	PyGiPolyline(const PyGiPolyline&) = delete;
	PyGiPolyline& operator=(const PyGiPolyline&) = delete;
	PyGiPolyline(PyGiPolyline&& other) noexcept : m_p(other.m_p) { other.m_p = nullptr; }

	AcGiPolyline* impObj() const { return m_p; }

	// ---- 读 ----
	unsigned int points() const;
	std::vector<AcGePoint3d> vertexList() const;
	std::optional<AcGeVector3d> normal() const;
	int linetypeGen() const;
	long long baseSubEntMarker() const;
	std::vector<long long> subEntMarkerList() const;
	bool hasGsMarker() const;
	bool isClosed() const;
	double elevation() const;
	double thickness() const;
	double constantWidth() const;
	std::vector<double> bulge() const;
	std::vector<double> width() const;
	bool hasBulges() const;
	bool hasWidth() const;
	std::vector<bool> arcSegmentFlags() const;
	std::optional<AcGePoint3d> getPointAt(unsigned int idx) const;
	std::optional<double> getBulgeAt(unsigned int idx) const;
	std::optional<std::pair<double, double>> getWidthsAt(unsigned int idx) const;

	// ---- 写 ----
	void setVertexList(const std::vector<AcGePoint3d>& pts);
	void setNormal(const std::optional<AcGeVector3d>& normal);
	void setLinetypeGen(int ltgen);
	void setBaseSubEntMarker(long long marker);
	void setSubEntMarkerList(const std::vector<long long>& markers);
	void setClosed(bool closed);
	void setElevation(double elevation);
	void setThickness(double thickness);
	void setConstantWidth(double width);
	void setBulge(const std::vector<double>& bulges);
	void setWidth(const std::vector<double>& widths);
	void setArcSegmentFlags(const std::vector<bool>& flags);

private:
	AcGiPolyline* m_p = nullptr;
	// SDK 的 set* 只保存裸指针、不做拷贝（bulge/width/arcFlags/markers 实测
	// 传入临时 vector 后读回是脏值），这里把数据常驻在包装对象里。
	std::vector<AcGePoint3d> m_verts;
	AcGeVector3d m_normal;
	bool m_hasNormal = false;
	std::vector<double> m_bulges;
	std::vector<double> m_widths;
	std::vector<char> m_arcFlags;
	std::vector<Adesk::LongPtr> m_markers;
};

void bind_PyGiPolyline(pym& m);
