#pragma once
// ===========================================================================
// PyGiMeshData.h —— mesh / shell 的逐边、逐面、逐顶点属性容器
// SDK: acgi.h AcGiEdgeData(1972) / AcGiFaceData(2002) / AcGiVertexData(2038)
//
// SDK 的 set* 只存裸指针、不拷贝、也没有长度入参（长度由 mesh/shell 的
// rows/columns/faceList 决定）。因此这里按 PyGiPolyline 的经验，由持有类
// 常驻 std::vector 保存 Python 传入的数据，setter 把 .data() 交给 SDK，
// getter 按自己记录的长度回读，避免“临时 vector 析构 → 悬空指针”。
//
// 运行期实测（AutoCAD 2024，瞬态上下文 kAcGiMain/DirectShortTerm/DirectTopmost）：
//   瞬态绘制上下文的 AcGiContext::supportsFillTrait() 恒为 false，
//   因此 polygon / mesh / shell / polyPolygon 在该上下文一律返回 Adesk::kFalse
//   （与是否携带 EdgeData/FaceData/VertexData 无关；point 顺序与数组长度已核对）。
//   这些带填充的图元只有在实体自己的 subWorldDraw（真实显示管线）里才有效，
//   届时本文件的四个容器对象即按上述方式把数组交给 SDK。
// ===========================================================================
#include "stdafx.h"
#include "PyDbTypeCaster.h"

class GiEdgeDataHolder
{
public:
	AcGiEdgeData& data() { return m_data; }
	const AcGiEdgeData& data() const { return m_data; }

	void setColors(const std::vector<short>& v);
	py::list colors() const;
	void setTrueColors(const std::vector<AcCmEntityColor>& v);
	py::list trueColors() const;
	void setLayers(const std::vector<AcDbObjectId>& v);
	py::list layers() const;
	void setLineTypes(const std::vector<AcDbObjectId>& v);
	py::list lineTypes() const;
	void setSelectionMarkers(const std::vector<Adesk::LongPtr>& v);
	py::list selectionMarkers() const;
	void setVisibility(const std::vector<int>& v);
	py::list visibility() const;

private:
	AcGiEdgeData m_data;
	std::vector<short> m_colors;
	std::vector<AcCmEntityColor> m_trueColors;
	std::vector<AcDbObjectId> m_layers;
	std::vector<AcDbObjectId> m_lineTypes;
	std::vector<Adesk::LongPtr> m_markers;
	std::vector<Adesk::UInt8> m_visibility;
	bool m_hasColors = false;
	bool m_hasTrueColors = false;
	bool m_hasLayers = false;
	bool m_hasLineTypes = false;
	bool m_hasMarkers = false;
	bool m_hasVisibility = false;
};


class GiFaceDataHolder
{
public:
	AcGiFaceData& data() { return m_data; }
	const AcGiFaceData& data() const { return m_data; }

	void setColors(const std::vector<short>& v);
	py::list colors() const;
	void setTrueColors(const std::vector<AcCmEntityColor>& v);
	py::list trueColors() const;
	void setLayers(const std::vector<AcDbObjectId>& v);
	py::list layers() const;
	void setSelectionMarkers(const std::vector<Adesk::LongPtr>& v);
	py::list selectionMarkers() const;
	void setNormals(py::object v);
	py::list normals() const;
	void setVisibility(const std::vector<int>& v);
	py::list visibility() const;
	void setMaterials(const std::vector<AcDbObjectId>& v);
	py::list materials() const;
	void setMappers(const std::vector<AcGiMapper>& v);
	py::list mappers() const;
	void setTransparency(const std::vector<AcCmTransparency>& v);
	py::list transparency() const;

private:
	AcGiFaceData m_data;
	std::vector<short> m_colors;
	std::vector<AcCmEntityColor> m_trueColors;
	std::vector<AcDbObjectId> m_layers;
	std::vector<Adesk::LongPtr> m_markers;
	std::vector<AcGeVector3d> m_normals;
	std::vector<Adesk::UInt8> m_visibility;
	std::vector<AcDbObjectId> m_materials;
	std::vector<AcGiMapper> m_mappers;
	std::vector<AcCmTransparency> m_transparency;
	bool m_hasColors = false;
	bool m_hasTrueColors = false;
	bool m_hasLayers = false;
	bool m_hasMarkers = false;
	bool m_hasNormals = false;
	bool m_hasVisibility = false;
	bool m_hasMaterials = false;
	bool m_hasMappers = false;
	bool m_hasTransparency = false;
};


class GiVertexDataHolder
{
public:
	AcGiVertexData& data() { return m_data; }
	const AcGiVertexData& data() const { return m_data; }

	void setNormals(py::object v);
	py::list normals() const;
	void setOrientationFlag(int flag);
	int orientationFlag() const;
	void setTrueColors(const std::vector<AcCmEntityColor>& v);
	py::list trueColors() const;
	// channel: 0 = kAllChannels, 1 = kMapChannels
	void setMappingCoords(int channel, py::object v);
	py::list mappingCoords(int channel) const;

private:
	AcGiVertexData m_data;
	std::vector<AcGeVector3d> m_normals;
	std::vector<AcCmEntityColor> m_trueColors;
	std::vector<AcGePoint3d> m_allChannelCoords;
	std::vector<AcGePoint3d> m_mapChannelCoords;
	bool m_hasNormals = false;
	bool m_hasTrueColors = false;
	bool m_hasAllChannelCoords = false;
	bool m_hasMapChannelCoords = false;
};

void bind_PyGiMeshData(pym& m);
