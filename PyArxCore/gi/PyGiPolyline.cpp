#include "stdafx.h"
#include "PyGiPolyline.h"
#include "PyGiTypes.h"

PyGiPolyline::PyGiPolyline() {
	m_p = new AcGiPolyline();
}

PyGiPolyline::PyGiPolyline(const std::vector<AcGePoint3d>& pts,
	const std::optional<AcGeVector3d>& normal,
	long long baseSubEntMarker) {
	std::vector<AcGePoint3d> verts = pts;      // SDK 需要可写指针
	if (verts.empty())
		throw py::value_error("AcGiPolyline 至少需要 1 个点");
	AcGeVector3d nrm = normal.value_or(AcGeVector3d::kZAxis);
	m_p = new AcGiPolyline(static_cast<Adesk::UInt32>(verts.size()),
		verts.data(),
		normal ? &nrm : nullptr,
		static_cast<Adesk::LongPtr>(baseSubEntMarker));
}

PyGiPolyline::~PyGiPolyline() {
	if (m_p) {
		delete m_p;
		m_p = nullptr;
	}
}

unsigned int PyGiPolyline::points() const {
	return static_cast<unsigned int>(m_p->points());
}

std::vector<AcGePoint3d> PyGiPolyline::vertexList() const {
	std::vector<AcGePoint3d> out;
	const Adesk::UInt32 n = m_p->points();
	const AcGePoint3d* v = m_p->vertexList();
	if (!v) return out;
	out.assign(v, v + n);
	return out;
}

std::optional<AcGeVector3d> PyGiPolyline::normal() const {
	if (const AcGeVector3d* n = m_p->normal())
		return *n;
	return std::nullopt;
}

int PyGiPolyline::linetypeGen() const {
	return static_cast<int>(m_p->linetypeGen());
}

long long PyGiPolyline::baseSubEntMarker() const {
	return static_cast<long long>(m_p->baseSubEntMarker());
}

std::vector<long long> PyGiPolyline::subEntMarkerList() const {
	std::vector<long long> out;
	const Adesk::UInt32 n = m_p->points();
	if (const Adesk::LongPtr* mk = m_p->subEntMarkerList()) {
		out.reserve(n);
		for (Adesk::UInt32 i = 0; i < n; ++i)
			out.push_back(static_cast<long long>(mk[i]));
	}
	return out;
}

bool PyGiPolyline::hasGsMarker() const { return m_p->hasGsMarker(); }
bool PyGiPolyline::isClosed() const { return m_p->isClosed(); }
double PyGiPolyline::elevation() const { return m_p->elevation(); }
double PyGiPolyline::thickness() const { return m_p->thickness(); }
double PyGiPolyline::constantWidth() const { return m_p->constantWidth(); }

std::vector<double> PyGiPolyline::bulge() const {
	std::vector<double> out;
	const Adesk::UInt32 n = m_p->points();
	if (const double* b = m_p->bulge()) {
		out.reserve(n);
		for (Adesk::UInt32 i = 0; i < n; ++i) out.push_back(b[i]);
	}
	return out;
}

std::vector<double> PyGiPolyline::width() const {
	std::vector<double> out;
	const Adesk::UInt32 n = m_p->points();
	if (const double* w = m_p->width()) {
		out.reserve(2 * n);
		// SDK: 每点 start/end 两个值
		for (Adesk::UInt32 i = 0; i < 2 * n; ++i) out.push_back(w[i]);
	}
	return out;
}

bool PyGiPolyline::hasBulges() const { return m_p->hasBulges(); }
bool PyGiPolyline::hasWidth() const { return m_p->hasWidth(); }

std::vector<bool> PyGiPolyline::arcSegmentFlags() const {
	std::vector<bool> out;
	const Adesk::UInt32 n = m_p->points();
	if (const bool* f = m_p->arcSegmentFlags()) {
		out.reserve(n);
		for (Adesk::UInt32 i = 0; i < n; ++i) out.push_back(f[i]);
	}
	return out;
}

std::optional<AcGePoint3d> PyGiPolyline::getPointAt(unsigned int idx) const {
	AcGePoint3d pt;
	if (m_p->getPointAt(idx, pt)) return pt;
	return std::nullopt;
}

std::optional<double> PyGiPolyline::getBulgeAt(unsigned int idx) const {
	double b = 0.0;
	if (m_p->getBulgeAt(idx, b)) return b;
	return std::nullopt;
}

std::optional<std::pair<double, double>> PyGiPolyline::getWidthsAt(unsigned int idx) const {
	double sw = 0.0, ew = 0.0;
	if (m_p->getWidthsAt(idx, sw, ew)) return std::make_pair(sw, ew);
	return std::nullopt;
}

void PyGiPolyline::setVertexList(const std::vector<AcGePoint3d>& pts) {
	if (pts.empty())
		throw py::value_error("setVertexList 需要至少 1 个点");
	m_verts = pts;
	m_p->setVertexList(static_cast<Adesk::UInt32>(m_verts.size()), m_verts.data());
}

void PyGiPolyline::setNormal(const std::optional<AcGeVector3d>& normal) {
	if (!normal) {
		m_hasNormal = false;
		m_p->setNormal(nullptr);
		return;
	}
	m_normal = *normal;
	m_hasNormal = true;
	m_p->setNormal(&m_normal);
}

void PyGiPolyline::setLinetypeGen(int ltgen) {
	m_p->setLinetypeGen(static_cast<AcGiPolyline::LinetypeGeneration>(ltgen));
}

void PyGiPolyline::setBaseSubEntMarker(long long marker) {
	m_p->setBaseSubEntMarker(static_cast<Adesk::LongPtr>(marker));
}

void PyGiPolyline::setSubEntMarkerList(const std::vector<long long>& markers) {
	m_markers.clear();
	m_markers.reserve(markers.size());
	for (long long v : markers) m_markers.push_back(static_cast<Adesk::LongPtr>(v));
	m_p->setSubEntMarkerList(m_markers.empty() ? nullptr : m_markers.data());
}

void PyGiPolyline::setClosed(bool closed) { m_p->setClosed(closed); }
void PyGiPolyline::setElevation(double elevation) { m_p->setElevation(elevation); }
void PyGiPolyline::setThickness(double thickness) { m_p->setThickness(thickness); }
void PyGiPolyline::setConstantWidth(double width) { m_p->setConstantWidth(width); }

void PyGiPolyline::setBulge(const std::vector<double>& bulges) {
	m_bulges = bulges;
	m_p->setBulge(m_bulges.empty() ? nullptr : m_bulges.data());
}

void PyGiPolyline::setWidth(const std::vector<double>& widths) {
	m_widths = widths;
	m_p->setWidth(m_widths.empty() ? nullptr : m_widths.data());
}

void PyGiPolyline::setArcSegmentFlags(const std::vector<bool>& flags) {
	m_arcFlags.clear();
	m_arcFlags.reserve(flags.size());
	for (bool f : flags) m_arcFlags.push_back(f ? 1 : 0);
	m_p->setArcSegmentFlags(m_arcFlags.empty() ? nullptr : reinterpret_cast<const bool*>(m_arcFlags.data()));
}

void bind_PyGiPolyline(pym& m) {
	py::enum_<AcGiPolyline::LinetypeGeneration>(m, "AcGiPolylineLinetypeGeneration")
		.value("kPerSegment", AcGiPolyline::kPerSegment)
		.value("kEndToEnd", AcGiPolyline::kEndToEnd);

	py::class_<PyGiPolyline>(m, "AcGiPolyline", "AcGi 折线参数（bulge/变宽/闭合/升程/厚度）")
		.def(py::init<>())
		.def(py::init([](py::object pts, py::object normal, long long baseMarker) {
			if (!normal.is_none())
				return PyGiPolyline(AsPoint3dList(pts, 1, "AcGiPolyline"), AsVector3d(normal), baseMarker);
			return PyGiPolyline(AsPoint3dList(pts, 1, "AcGiPolyline"), std::nullopt, baseMarker);
		}), py::arg("points"), py::arg("normal") = py::none(), py::arg("base_subent_marker") = -1)
		.def("points", &PyGiPolyline::points)
		.def("vertexList", &PyGiPolyline::vertexList)
		.def("normal", &PyGiPolyline::normal)
		.def("linetypeGen", &PyGiPolyline::linetypeGen)
		.def("baseSubEntMarker", &PyGiPolyline::baseSubEntMarker)
		.def("subEntMarkerList", &PyGiPolyline::subEntMarkerList)
		.def("hasGsMarker", &PyGiPolyline::hasGsMarker)
		.def("isClosed", &PyGiPolyline::isClosed)
		.def("elevation", &PyGiPolyline::elevation)
		.def("thickness", &PyGiPolyline::thickness)
		.def("constantWidth", &PyGiPolyline::constantWidth)
		.def("bulge", &PyGiPolyline::bulge)
		.def("width", &PyGiPolyline::width)
		.def("hasBulges", &PyGiPolyline::hasBulges)
		.def("hasWidth", &PyGiPolyline::hasWidth)
		.def("arcSegmentFlags", &PyGiPolyline::arcSegmentFlags)
		.def("getPointAt", &PyGiPolyline::getPointAt, py::arg("index"))
		.def("getBulgeAt", &PyGiPolyline::getBulgeAt, py::arg("index"))
		.def("getWidthsAt", &PyGiPolyline::getWidthsAt, py::arg("index"))
		.def("setVertexList", [](PyGiPolyline& p, py::object pts) { p.setVertexList(AsPoint3dList(pts, 1, "setVertexList")); }, py::arg("points"))
		.def("setNormal", [](PyGiPolyline& p, py::object normal) {
			if (normal.is_none()) p.setNormal(std::nullopt);
			else p.setNormal(AsVector3d(normal));
		}, py::arg("normal"))
		.def("setLinetypeGen", &PyGiPolyline::setLinetypeGen, py::arg("linetype_gen"))
		.def("setBaseSubEntMarker", &PyGiPolyline::setBaseSubEntMarker, py::arg("marker"))
		.def("setSubEntMarkerList", &PyGiPolyline::setSubEntMarkerList, py::arg("markers"))
		.def("setClosed", &PyGiPolyline::setClosed, py::arg("closed"))
		.def("setElevation", &PyGiPolyline::setElevation, py::arg("elevation"))
		.def("setThickness", &PyGiPolyline::setThickness, py::arg("thickness"))
		.def("setConstantWidth", &PyGiPolyline::setConstantWidth, py::arg("width"))
		.def("setBulge", &PyGiPolyline::setBulge, py::arg("bulges"))
		.def("setWidth", &PyGiPolyline::setWidth, py::arg("widths"))
		.def("setArcSegmentFlags", &PyGiPolyline::setArcSegmentFlags, py::arg("flags"));
}
