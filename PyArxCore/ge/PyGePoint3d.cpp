#include "stdafx.h"
#include "PyGePoint3d.h"

#include <format>
#include <string>
#include <array>

void bind_AcGePoint3d(pybind11::module& m) {
	py::class_<AcGePoint3d>(m, "AcGePoint3d", py::buffer_protocol())
		// 缓冲区协议：将对象映射为连续内存

		.def(py::init<>(), "Default constructor (creates point at origin)")
		.def(py::init<double, double, double>(),
			py::arg("x"), py::arg("y"), py::arg("z"),
			"Construct from XYZ coordinates")
		.def(py::init<const AcGePoint3d&>(), py::arg("src"))

		.def(py::init<const AcGePlanarEnt&, const AcGePoint2d&>(),
			py::arg("plane"), py::arg("point2d"),
			"Construct from planar surface and 2D point coordinates")
		.def_buffer(
			[](AcGePoint3d& p) -> py::buffer_info {
				return py::buffer_info(&p.x, sizeof(double), py::format_descriptor<double>::format(), 1, { 3 }, { sizeof(double) });
			})
		// Constants
		.def_readonly_static("kOrigin", &AcGePoint3d::kOrigin, "Global origin point constant (0,0,0)")

		// Geometric transformations
		.def("transformBy", &AcGePoint3d::transformBy,
			py::arg("matrix"),
			"Transform point using 3D transformation matrix")
		.def("rotateBy", &AcGePoint3d::rotateBy,
			py::arg("angle"), py::arg("axis"), py::arg("wrtPoint") = AcGePoint3d::kOrigin,
			"Rotate point around axis vector relative to reference point")
		.def("mirror", &AcGePoint3d::mirror,
			py::arg("plane"),
			"Mirror point about a plane")
		.def("project", &AcGePoint3d::project,
			py::arg("plane"), py::arg("direction"),
			"Project point onto plane along direction vector")
		.def("orthoProject", &AcGePoint3d::orthoProject,
			py::arg("plane"),
			"Orthogonal projection onto plane")

		// Operators
		.def("__add__", py::overload_cast<const AcGeVector3d&>(&AcGePoint3d::operator+, py::const_),
			py::arg("vector"),
			"Translate point by vector addition")
		// [ADD] in-place vector addition (SDK operator += )
		.def("__iadd__", [](AcGePoint3d& p, const AcGeVector3d& v) -> AcGePoint3d& {
		return p += v;
			}, py::arg("vector"),
			"Translate this point in place by a vector")
		.def("__sub__", py::overload_cast<const AcGeVector3d&>(&AcGePoint3d::operator-, py::const_),
			py::arg("vector"),
			"Translate point by vector subtraction")
		// [ADD] in-place vector subtraction (SDK operator -= )
		.def("__isub__", [](AcGePoint3d& p, const AcGeVector3d& v) -> AcGePoint3d& {
		return p -= v;
			}, py::arg("vector"),
			"Translate this point in place by subtracting a vector")
		.def("__sub__", py::overload_cast<const AcGePoint3d&>(&AcGePoint3d::operator-, py::const_),
			py::arg("point"),
			"Get displacement vector between two points")
		.def("__mul__", py::overload_cast<double>(&AcGePoint3d::operator*, py::const_),
			py::arg("scale"),
			"Uniform scaling of coordinates")
		.def("__rmul__", py::overload_cast<double>(&AcGePoint3d::operator*, py::const_),
			py::arg("scale"),
			"Right-hand uniform scaling")
		// [ADD] in-place scalar multiply / divide (SDK operator *= /= )
		.def("__imul__", [](AcGePoint3d& p, double s) -> AcGePoint3d& {
		return p *= s;
			}, py::arg("scale"),
			"Scale coordinates in place")
		.def("__itruediv__", [](AcGePoint3d& p, double s) -> AcGePoint3d& {
		return p /= s;
			}, py::arg("divisor"),
			"Divide coordinates in place by scalar")
		.def("__truediv__", &AcGePoint3d::operator/,
			py::arg("divisor"),
			"Divide coordinates by scalar")

		// Core methods
		.def("distanceTo", &AcGePoint3d::distanceTo,
			py::arg("point"),
			"Calculate Euclidean distance to another point")
		.def("asVector", &AcGePoint3d::asVector,
			"Convert to vector from origin")

		// Scaling / equality
		.def("scaleBy", &AcGePoint3d::scaleBy,
			py::arg("scaleFactor"), py::arg("wrtPoint") = AcGePoint3d::kOrigin,
			"Scale point about a reference point")
		.def("setToProduct", &AcGePoint3d::setToProduct,
			py::arg("matrix"), py::arg("point"),
			"Set this point to matrix * point product")
		// [ADD] SDK setToSum: this = point + vector
		.def("setToSum", &AcGePoint3d::setToSum,
			py::arg("point"), py::arg("vector"),
			"Set this point to point + vector")
		.def("__eq__", &AcGePoint3d::operator==, py::arg("pnt"))
		.def("__ne__", &AcGePoint3d::operator!=, py::arg("pnt"))
		.def("isEqualTo", &AcGePoint3d::isEqualTo,
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)

		// Data access
		.def_readwrite("x", &AcGePoint3d::x, "X coordinate component")
		.def_readwrite("y", &AcGePoint3d::y, "Y coordinate component")
		.def_readwrite("z", &AcGePoint3d::z, "Z coordinate component")
		.def("set", py::overload_cast<double, double, double>(&AcGePoint3d::set),
			py::arg("x"), py::arg("y"), py::arg("z"),
			"Set new XYZ coordinates")
		.def("set", py::overload_cast<const AcGePlanarEnt&, const AcGePoint2d&>(&AcGePoint3d::set),
			py::arg("plane"), py::arg("point2d"),
			"Set this point from a 2D point expressed on the given planar surface")
		.def("__getitem__", [](const AcGePoint3d& p, int i) { return p[i]; },
			py::arg("index"),
			"Access coordinate by index (0=x, 1=y, 2=z)")
		// [ADD] indexed coordinate assignment (SDK operator [] write)
		.def("__setitem__", [](AcGePoint3d& p, int i, double val) {
		p[i] = val;
			}, py::arg("index"), py::arg("value"),
			"Assign coordinate by index (0=x, 1=y, 2=z)")
		// [ADD] SDK convert2d: project this point onto a planar surface
		.def("convert2d", &AcGePoint3d::convert2d,
			py::arg("plane"),
		"Convert this 3D point to 2D coordinates on the given planar surface")
		// [ADD] SDK toArray: return coordinates as a length-3 tuple
		.def("toArray", [](const AcGePoint3d& p) {
		auto a = p.toArray();
		return py::make_tuple(a[0], a[1], a[2]);
			}, "Return the XYZ coordinates as a tuple of 3 doubles")
		// String representation
		.def("__repr__", [](const AcGePoint3d& p) {
		return std::format("AcGePoint3d({:.3f}, {:.3f},{:.3f})", p.x, p.y, p.z);
			});
}
