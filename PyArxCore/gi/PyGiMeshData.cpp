// ===========================================================================
// PyGiMeshData.cpp —— AcGiEdgeData / AcGiFaceData / AcGiVertexData 绑定
// 见 PyGiMeshData.h 顶部说明：数据由持有类常驻，SDK 只拿指针。
// ===========================================================================
#include "stdafx.h"
#include "PyGiMeshData.h"
#include "PyGiTypes.h"

// Python 序列 -> AcGeVector3d / AcGePoint3d 列表（同时接受 AcGe 对象与 (x,y,z) 元组）
static std::vector<AcGeVector3d> AsVector3dList(const py::object& obj)
{
	std::vector<AcGeVector3d> out;
	const py::sequence seq = py::cast<py::sequence>(obj);
	const size_t n = static_cast<size_t>(py::len(seq));
	out.reserve(n);
	for (size_t i = 0; i < n; ++i) {
		py::object item = seq[static_cast<py::ssize_t>(i)];
		out.push_back(AsVector3d(item));
	}
	return out;
}

static std::vector<AcGePoint3d> AsPoint3dListLocal(const py::object& obj)
{
	std::vector<AcGePoint3d> out;
	const py::sequence seq = py::cast<py::sequence>(obj);
	const size_t n = static_cast<size_t>(py::len(seq));
	out.reserve(n);
	for (size_t i = 0; i < n; ++i) {
		py::object item = seq[static_cast<py::ssize_t>(i)];
		out.push_back(AsPoint3d(item));
	}
	return out;
}

// ---------------------------------------------------------------------------
// AcGiEdgeData
// ---------------------------------------------------------------------------
void GiEdgeDataHolder::setColors(const std::vector<short>& v)
{
	m_colors = v;
	m_hasColors = !v.empty();
	m_data.setColors(m_hasColors ? m_colors.data() : nullptr);
}

py::list GiEdgeDataHolder::colors() const
{
	py::list out;
	if (!m_hasColors) return out;
	const short* p = m_data.colors();
	if (!p) return out;
	for (size_t i = 0; i < m_colors.size(); ++i) out.append(py::cast(p[i]));
	return out;
}

void GiEdgeDataHolder::setTrueColors(const std::vector<AcCmEntityColor>& v)
{
	m_trueColors = v;
	m_hasTrueColors = !v.empty();
	m_data.setTrueColors(m_hasTrueColors ? m_trueColors.data() : nullptr);
}

py::list GiEdgeDataHolder::trueColors() const
{
	py::list out;
	if (!m_hasTrueColors) return out;
	const AcCmEntityColor* p = m_data.trueColors();
	if (!p) return out;
	for (size_t i = 0; i < m_trueColors.size(); ++i) out.append(py::cast(p[i]));
	return out;
}

void GiEdgeDataHolder::setLayers(const std::vector<AcDbObjectId>& v)
{
	m_layers = v;
	m_hasLayers = !v.empty();
	m_data.setLayers(m_hasLayers ? m_layers.data() : nullptr);
}

py::list GiEdgeDataHolder::layers() const
{
	py::list out;
	if (!m_hasLayers) return out;
	const AcDbObjectId* p = m_data.layerIds();
	if (!p) return out;
	for (size_t i = 0; i < m_layers.size(); ++i) out.append(py::int_((long long)p[i].asOldId()));
	return out;
}

void GiEdgeDataHolder::setLineTypes(const std::vector<AcDbObjectId>& v)
{
	m_lineTypes = v;
	m_hasLineTypes = !v.empty();
	m_data.setLineTypes(m_hasLineTypes ? m_lineTypes.data() : nullptr);
}

py::list GiEdgeDataHolder::lineTypes() const
{
	py::list out;
	if (!m_hasLineTypes) return out;
	const AcDbObjectId* p = m_data.linetypeIds();
	if (!p) return out;
	for (size_t i = 0; i < m_lineTypes.size(); ++i) out.append(py::int_((long long)p[i].asOldId()));
	return out;
}

void GiEdgeDataHolder::setSelectionMarkers(const std::vector<Adesk::LongPtr>& v)
{
	m_markers = v;
	m_hasMarkers = !v.empty();
	m_data.setSelectionMarkers(m_hasMarkers ? m_markers.data() : nullptr);
}

py::list GiEdgeDataHolder::selectionMarkers() const
{
	py::list out;
	if (!m_hasMarkers) return out;
	const Adesk::LongPtr* p = m_data.selectionMarkers();
	if (!p) return out;
	for (size_t i = 0; i < m_markers.size(); ++i) out.append(py::cast(p[i]));
	return out;
}

void GiEdgeDataHolder::setVisibility(const std::vector<int>& v)
{
	m_visibility.assign(v.begin(), v.end());
	m_hasVisibility = !v.empty();
	m_data.setVisibility(m_hasVisibility ? m_visibility.data() : nullptr);
}

py::list GiEdgeDataHolder::visibility() const
{
	py::list out;
	if (!m_hasVisibility) return out;
	const Adesk::UInt8* p = m_data.visibility();
	if (!p) return out;
	for (size_t i = 0; i < m_visibility.size(); ++i) out.append(py::cast((int)p[i]));
	return out;
}

// ---------------------------------------------------------------------------
// AcGiFaceData
// ---------------------------------------------------------------------------
void GiFaceDataHolder::setColors(const std::vector<short>& v)
{
	m_colors = v;
	m_hasColors = !v.empty();
	m_data.setColors(m_hasColors ? m_colors.data() : nullptr);
}

py::list GiFaceDataHolder::colors() const
{
	py::list out;
	if (!m_hasColors) return out;
	const short* p = m_data.colors();
	if (!p) return out;
	for (size_t i = 0; i < m_colors.size(); ++i) out.append(py::cast(p[i]));
	return out;
}

void GiFaceDataHolder::setTrueColors(const std::vector<AcCmEntityColor>& v)
{
	m_trueColors = v;
	m_hasTrueColors = !v.empty();
	m_data.setTrueColors(m_hasTrueColors ? m_trueColors.data() : nullptr);
}

py::list GiFaceDataHolder::trueColors() const
{
	py::list out;
	if (!m_hasTrueColors) return out;
	const AcCmEntityColor* p = m_data.trueColors();
	if (!p) return out;
	for (size_t i = 0; i < m_trueColors.size(); ++i) out.append(py::cast(p[i]));
	return out;
}

void GiFaceDataHolder::setLayers(const std::vector<AcDbObjectId>& v)
{
	m_layers = v;
	m_hasLayers = !v.empty();
	m_data.setLayers(m_hasLayers ? m_layers.data() : nullptr);
}

py::list GiFaceDataHolder::layers() const
{
	py::list out;
	if (!m_hasLayers) return out;
	const AcDbObjectId* p = m_data.layerIds();
	if (!p) return out;
	for (size_t i = 0; i < m_layers.size(); ++i) out.append(py::int_((long long)p[i].asOldId()));
	return out;
}

void GiFaceDataHolder::setSelectionMarkers(const std::vector<Adesk::LongPtr>& v)
{
	m_markers = v;
	m_hasMarkers = !v.empty();
	m_data.setSelectionMarkers(m_hasMarkers ? m_markers.data() : nullptr);
}

py::list GiFaceDataHolder::selectionMarkers() const
{
	py::list out;
	if (!m_hasMarkers) return out;
	const Adesk::LongPtr* p = m_data.selectionMarkers();
	if (!p) return out;
	for (size_t i = 0; i < m_markers.size(); ++i) out.append(py::cast(p[i]));
	return out;
}

void GiFaceDataHolder::setNormals(py::object v)
{
	m_normals = AsVector3dList(v);
	m_hasNormals = !m_normals.empty();
	m_data.setNormals(m_hasNormals ? m_normals.data() : nullptr);
}

py::list GiFaceDataHolder::normals() const
{
	py::list out;
	if (!m_hasNormals) return out;
	const AcGeVector3d* p = m_data.normals();
	if (!p) return out;
	for (size_t i = 0; i < m_normals.size(); ++i) out.append(py::cast(p[i]));
	return out;
}

void GiFaceDataHolder::setVisibility(const std::vector<int>& v)
{
	m_visibility.assign(v.begin(), v.end());
	m_hasVisibility = !v.empty();
	m_data.setVisibility(m_hasVisibility ? m_visibility.data() : nullptr);
}

py::list GiFaceDataHolder::visibility() const
{
	py::list out;
	if (!m_hasVisibility) return out;
	const Adesk::UInt8* p = m_data.visibility();
	if (!p) return out;
	for (size_t i = 0; i < m_visibility.size(); ++i) out.append(py::cast((int)p[i]));
	return out;
}

void GiFaceDataHolder::setMaterials(const std::vector<AcDbObjectId>& v)
{
	m_materials = v;
	m_hasMaterials = !v.empty();
	m_data.setMaterials(m_hasMaterials ? m_materials.data() : nullptr);
}

py::list GiFaceDataHolder::materials() const
{
	py::list out;
	if (!m_hasMaterials) return out;
	const AcDbObjectId* p = m_data.materials();
	if (!p) return out;
	for (size_t i = 0; i < m_materials.size(); ++i) out.append(py::int_((long long)p[i].asOldId()));
	return out;
}

void GiFaceDataHolder::setMappers(const std::vector<AcGiMapper>& v)
{
	m_mappers = v;
	m_hasMappers = !v.empty();
	m_data.setMappers(m_hasMappers ? m_mappers.data() : nullptr);
}

py::list GiFaceDataHolder::mappers() const
{
	py::list out;
	if (!m_hasMappers) return out;
	const AcGiMapper* p = m_data.mappers();
	if (!p) return out;
	for (size_t i = 0; i < m_mappers.size(); ++i) out.append(py::cast(p[i]));
	return out;
}

void GiFaceDataHolder::setTransparency(const std::vector<AcCmTransparency>& v)
{
	m_transparency = v;
	m_hasTransparency = !v.empty();
	m_data.setTransparency(m_hasTransparency ? m_transparency.data() : nullptr);
}

py::list GiFaceDataHolder::transparency() const
{
	py::list out;
	if (!m_hasTransparency) return out;
	const AcCmTransparency* p = m_data.transparency();
	if (!p) return out;
	for (size_t i = 0; i < m_transparency.size(); ++i) out.append(py::cast(p[i]));
	return out;
}

// ---------------------------------------------------------------------------
// AcGiVertexData
// ---------------------------------------------------------------------------
void GiVertexDataHolder::setNormals(py::object v)
{
	m_normals = AsVector3dList(v);
	m_hasNormals = !m_normals.empty();
	m_data.setNormals(m_hasNormals ? m_normals.data() : nullptr);
}

py::list GiVertexDataHolder::normals() const
{
	py::list out;
	if (!m_hasNormals) return out;
	const AcGeVector3d* p = m_data.normals();
	if (!p) return out;
	for (size_t i = 0; i < m_normals.size(); ++i) out.append(py::cast(p[i]));
	return out;
}

void GiVertexDataHolder::setOrientationFlag(int flag)
{
	m_data.setOrientationFlag(static_cast<AcGiOrientationType>(flag));
}

int GiVertexDataHolder::orientationFlag() const
{
	return static_cast<int>(m_data.orientationFlag());
}

void GiVertexDataHolder::setTrueColors(const std::vector<AcCmEntityColor>& v)
{
	m_trueColors = v;
	m_hasTrueColors = !v.empty();
	m_data.setTrueColors(m_hasTrueColors ? m_trueColors.data() : nullptr);
}

py::list GiVertexDataHolder::trueColors() const
{
	py::list out;
	if (!m_hasTrueColors) return out;
	const AcCmEntityColor* p = m_data.trueColors();
	if (!p) return out;
	for (size_t i = 0; i < m_trueColors.size(); ++i) out.append(py::cast(p[i]));
	return out;
}

void GiVertexDataHolder::setMappingCoords(int channel, py::object v)
{
	using MC = AcGiVertexData::MapChannel;
	auto coords = AsPoint3dListLocal(v);
	const MC ch = (channel == 1) ? MC::kMapChannels : MC::kAllChannels;
	if (ch == MC::kMapChannels) {
		m_mapChannelCoords = coords;
		m_hasMapChannelCoords = !coords.empty();
		m_data.setMappingCoords(ch, m_hasMapChannelCoords ? m_mapChannelCoords.data() : nullptr);
	} else {
		m_allChannelCoords = coords;
		m_hasAllChannelCoords = !coords.empty();
		m_data.setMappingCoords(ch, m_hasAllChannelCoords ? m_allChannelCoords.data() : nullptr);
	}
}

py::list GiVertexDataHolder::mappingCoords(int channel) const
{
	using MC = AcGiVertexData::MapChannel;
	py::list out;
	const MC ch = (channel == 1) ? MC::kMapChannels : MC::kAllChannels;
	bool has = (ch == MC::kMapChannels) ? m_hasMapChannelCoords : m_hasAllChannelCoords;
	if (!has) return out;
	const AcGePoint3d* p = m_data.mappingCoords(ch);
	if (!p) return out;
	const size_t n = (ch == MC::kMapChannels) ? m_mapChannelCoords.size() : m_allChannelCoords.size();
	for (size_t i = 0; i < n; ++i) out.append(py::cast(p[i]));
	return out;
}

// ---------------------------------------------------------------------------
// 绑定
// ---------------------------------------------------------------------------
void bind_PyGiMeshData(pym& m)
{
	py::enum_<AcGiVertexData::MapChannel>(m, "AcGiMapChannel")
		.value("kAllChannels", AcGiVertexData::kAllChannels)
		.value("kMapChannels", AcGiVertexData::kMapChannels);

	py::class_<GiEdgeDataHolder>(m, "AcGiEdgeData",
		"mesh/shell 逐边属性（颜色/真彩色/图层/线型/选择标记/可见性）")
		.def(py::init<>())
		.def("setColors", &GiEdgeDataHolder::setColors)
		.def("colors", &GiEdgeDataHolder::colors)
		.def("setTrueColors", &GiEdgeDataHolder::setTrueColors)
		.def("trueColors", &GiEdgeDataHolder::trueColors)
		.def("setLayers", &GiEdgeDataHolder::setLayers)
		.def("layers", &GiEdgeDataHolder::layers)
		.def("setLineTypes", &GiEdgeDataHolder::setLineTypes)
		.def("lineTypes", &GiEdgeDataHolder::lineTypes)
		.def("setSelectionMarkers", &GiEdgeDataHolder::setSelectionMarkers)
		.def("selectionMarkers", &GiEdgeDataHolder::selectionMarkers)
		.def("setVisibility", &GiEdgeDataHolder::setVisibility)
		.def("visibility", &GiEdgeDataHolder::visibility);

	py::class_<GiFaceDataHolder>(m, "AcGiFaceData",
		"mesh/shell 逐面属性（颜色/真彩色/图层/法向/可见性/材质/映射/透明度）")
		.def(py::init<>())
		.def("setColors", &GiFaceDataHolder::setColors)
		.def("colors", &GiFaceDataHolder::colors)
		.def("setTrueColors", &GiFaceDataHolder::setTrueColors)
		.def("trueColors", &GiFaceDataHolder::trueColors)
		.def("setLayers", &GiFaceDataHolder::setLayers)
		.def("layers", &GiFaceDataHolder::layers)
		.def("setSelectionMarkers", &GiFaceDataHolder::setSelectionMarkers)
		.def("selectionMarkers", &GiFaceDataHolder::selectionMarkers)
		.def("setNormals", &GiFaceDataHolder::setNormals)
		.def("normals", &GiFaceDataHolder::normals)
		.def("setVisibility", &GiFaceDataHolder::setVisibility)
		.def("visibility", &GiFaceDataHolder::visibility)
		.def("setMaterials", &GiFaceDataHolder::setMaterials)
		.def("materials", &GiFaceDataHolder::materials)
		.def("setMappers", &GiFaceDataHolder::setMappers)
		.def("mappers", &GiFaceDataHolder::mappers)
		.def("setTransparency", &GiFaceDataHolder::setTransparency)
		.def("transparency", &GiFaceDataHolder::transparency);

	py::class_<GiVertexDataHolder>(m, "AcGiVertexData",
		"mesh/shell 逐顶点属性（法向/朝向标志/真彩色/贴图坐标）")
		.def(py::init<>())
		.def("setNormals", &GiVertexDataHolder::setNormals)
		.def("normals", &GiVertexDataHolder::normals)
		.def("setOrientationFlag", &GiVertexDataHolder::setOrientationFlag)
		.def("orientationFlag", &GiVertexDataHolder::orientationFlag)
		.def("setTrueColors", &GiVertexDataHolder::setTrueColors)
		.def("trueColors", &GiVertexDataHolder::trueColors)
		.def("setMappingCoords", &GiVertexDataHolder::setMappingCoords)
		.def("mappingCoords", &GiVertexDataHolder::mappingCoords);
}
