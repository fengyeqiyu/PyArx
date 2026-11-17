#include "PyDbValueTypes.h"
#include "PyDbExtents.h"
#include "dbid.h"

namespace
{
	// PyRx Db.IdPair：ARX 无对应类型，PyRx 自造（两枚 ObjectId）
	struct PyDbIdPair
	{
		AcDbObjectId first;
		AcDbObjectId second;
		PyDbIdPair() = default;
		PyDbIdPair(const AcDbObjectId& a, const AcDbObjectId& b) : first(a), second(b) {}
		bool operator==(const PyDbIdPair& o) const
		{
			return first == o.first && second == o.second;
		}
		std::string repr() const
		{
			return "IdPair(" + std::to_string(first.asOldId()) + ", " + std::to_string(second.asOldId()) + ")";
		}
	};
}

void bind_PyDbValueTypes(pym& m)
{
	// ---------------- Extents（3d） ----------------
	if (!py::hasattr(m, "Extents"))
	{
		py::class_<PyDbExtents3d>(m, "Extents", "AcDbExtents 封装（对齐 PyRx Db.Extents）")
			.def(py::init<>())
			.def(py::init([](const AcGePoint3d& min, const AcGePoint3d& max) { return PyDbExtents3d(min, max); }),
				py::arg("min"), py::arg("max"))
			.def("minPoint", &PyDbExtents3d::minPoint)
			.def("maxPoint", &PyDbExtents3d::maxPoint)
			.def("midPoint", [](const PyDbExtents3d& e) {
				AcGePoint3d a = e.minPoint(), b = e.maxPoint();
				return AcGePoint3d((a.x + b.x) * 0.5, (a.y + b.y) * 0.5, (a.z + b.z) * 0.5);
			})
			.def("isEmpty", &PyDbExtents3d::isEmpty)
			.def("set", &PyDbExtents3d::set, py::arg("min"), py::arg("max"))
			.def("addPoint", &PyDbExtents3d::addPoint, py::arg("pt"))
			.def("addPoints", [](PyDbExtents3d& e, const py::iterable& pts) {
				for (auto h : pts)
					e.addPoint(py::cast<AcGePoint3d>(h));
			}, py::arg("points"))
			.def("addExt", &PyDbExtents3d::addExt, py::arg("ext"))
			.def("addBlockExt", [](PyDbExtents3d& e, AcDbBlockTableRecord* pBtr) {
				return e.addBlockExt(pBtr);
			}, py::arg("blockTableRecord"))
			.def("expandBy", &PyDbExtents3d::expandBy, py::arg("vec"))
			.def("transformBy", &PyDbExtents3d::transformBy, py::arg("mat"))
			.def("coords", [](const PyDbExtents3d& e) {
				AcGePoint3d a = e.minPoint(), b = e.maxPoint();
				return py::make_tuple(a.x, a.y, a.z, b.x, b.y, b.z);
			})
			.def("contains", [](const PyDbExtents3d& e, const AcGePoint3d& p) {
				if (e.isEmpty())
					return false;
				AcGePoint3d a = e.minPoint(), b = e.maxPoint();
				return p.x >= a.x && p.x <= b.x && p.y >= a.y && p.y <= b.y && p.z >= a.z && p.z <= b.z;
			}, py::arg("pt"))
			.def("intersectsWith", [](const PyDbExtents3d& e, const PyDbExtents3d& o) {
				if (e.isEmpty() || o.isEmpty())
					return false;
				AcGePoint3d a1 = e.minPoint(), b1 = e.maxPoint(), a2 = o.minPoint(), b2 = o.maxPoint();
				return a1.x <= b2.x && b1.x >= a2.x && a1.y <= b2.y && b1.y >= a2.y && a1.z <= b2.z && b1.z >= a2.z;
			}, py::arg("other"))
			.def("__eq__", &PyDbExtents3d::operator==, py::is_operator())
			.def("__repr__", [](const PyDbExtents3d& e) {
				if (e.isEmpty())
					return std::string("Extents(empty)");
				AcGePoint3d a = e.minPoint(), b = e.maxPoint();
				char buf[160];
				snprintf(buf, sizeof(buf), "Extents((%g,%g,%g), (%g,%g,%g))", a.x, a.y, a.z, b.x, b.y, b.z);
				return std::string(buf);
			});
	}

	// ---------------- Extents2d ----------------
	if (!py::hasattr(m, "Extents2d"))
	{
		py::class_<PyDbExtents2d>(m, "Extents2d", "AcDbExtents2d 封装（对齐 PyRx Db.Extents2d）")
			.def(py::init<>())
			.def(py::init([](const AcGePoint2d& min, const AcGePoint2d& max) { return PyDbExtents2d(min, max); }),
				py::arg("min"), py::arg("max"))
			.def("minPoint", &PyDbExtents2d::minPoint)
			.def("maxPoint", &PyDbExtents2d::maxPoint)
			.def("midPoint", [](const PyDbExtents2d& e) {
				AcGePoint2d a = e.minPoint(), b = e.maxPoint();
				return AcGePoint2d((a.x + b.x) * 0.5, (a.y + b.y) * 0.5);
			})
			.def("set", &PyDbExtents2d::set, py::arg("min"), py::arg("max"))
			.def("addPoint", &PyDbExtents2d::addPoint, py::arg("pt"))
			.def("addPoints", [](PyDbExtents2d& e, const py::iterable& pts) {
				for (auto h : pts)
					e.addPoint(py::cast<AcGePoint2d>(h));
			}, py::arg("points"))
			.def("addExt", &PyDbExtents2d::addExt, py::arg("ext"))
			.def("expandBy", &PyDbExtents2d::expandBy, py::arg("vec"))
			.def("transformBy", &PyDbExtents2d::transformBy, py::arg("mat"))
			.def("coords", [](const PyDbExtents2d& e) {
				AcGePoint2d a = e.minPoint(), b = e.maxPoint();
				return py::make_tuple(a.x, a.y, b.x, b.y);
			})
			.def("contains", [](const PyDbExtents2d& e, const AcGePoint2d& p) {
				AcGePoint2d a = e.minPoint(), b = e.maxPoint();
				return p.x >= a.x && p.x <= b.x && p.y >= a.y && p.y <= b.y;
			}, py::arg("pt"))
			.def("intersectsWith", [](const PyDbExtents2d& e, const PyDbExtents2d& o) {
				AcGePoint2d a1 = e.minPoint(), b1 = e.maxPoint(), a2 = o.minPoint(), b2 = o.maxPoint();
				return a1.x <= b2.x && b1.x >= a2.x && a1.y <= b2.y && b1.y >= a2.y;
			}, py::arg("other"))
			.def("__eq__", &PyDbExtents2d::operator==, py::is_operator())
			.def("__repr__", [](const PyDbExtents2d& e) {
				AcGePoint2d a = e.minPoint(), b = e.maxPoint();
				char buf[128];
				snprintf(buf, sizeof(buf), "Extents2d((%g,%g), (%g,%g))", a.x, a.y, b.x, b.y);
				return std::string(buf);
			});
	}

	// ---------------- ObjectIdArray ----------------
	// PyArrayCaster.h 已给 AcDbObjectIdArray 做了 type_caster 全特化（Python 侧就是 oldId 的 list），
	// pybind11 不允许同一类型既 class_ 注册又自定义 caster（见 PyArrayCaster.h:83-85 的冲突说明），
	// 所以这里不注册类，直接把 PyRx 的 ObjectIdArray 对到内置 list。
	if (!py::hasattr(m, "ObjectIdArray"))
		m.attr("ObjectIdArray") = py::module_::import("builtins").attr("list");

	if (!py::hasattr(m, "IdPair"))
	{
		py::class_<PyDbIdPair>(m, "IdPair", "两枚 ObjectId 的组合（对齐 PyRx Db.IdPair）")
			.def(py::init<>())
			.def(py::init([](const AcDbObjectId& a, const AcDbObjectId& b) { return PyDbIdPair(a, b); }),
				py::arg("first"), py::arg("second"))
			.def_readwrite("first", &PyDbIdPair::first)
			.def_readwrite("second", &PyDbIdPair::second)
			.def("__eq__", &PyDbIdPair::operator==, py::is_operator())
			.def("__repr__", &PyDbIdPair::repr);
	}

	// 兼容别名（PyArx 既有命名习惯）
	if (py::hasattr(m, "Extents"))
		m.attr("PyDbExtents") = m.attr("Extents");
	if (py::hasattr(m, "Extents2d"))
		m.attr("PyDbExtents2d") = m.attr("Extents2d");
	if (py::hasattr(m, "ObjectIdArray"))
		m.attr("PyDbObjectIdArray") = m.attr("ObjectIdArray");
}
