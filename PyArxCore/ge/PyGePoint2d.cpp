#include "stdafx.h"
#include "PyGePoint2d.h"

#include <format>
#include <string>

void bind_AcGePoint2d(pybind11::module& m) {
	py::class_<AcGePoint2d>(m, "AcGePoint2d")
		// Constructors
		.def(py::init<>())
		.def(py::init<const AcGePoint2d&>(), py::arg("pnt"))
		.def(py::init<double, double>(), py::arg("x"), py::arg("y"))

		// Static constants
		.def_readonly_static("kOrigin", &AcGePoint2d::kOrigin)

		// Matrix transformations
		.def("setToProduct", &AcGePoint2d::setToProduct,
			py::arg("mat"), py::arg("pnt"))
		.def("transformBy", &AcGePoint2d::transformBy,
			py::arg("matrix"))
		.def("rotateBy", &AcGePoint2d::rotateBy,
			py::arg("angle"), py::arg("wrtPoint") = AcGePoint2d::kOrigin)
		.def("mirror", &AcGePoint2d::mirror,
			py::arg("line"))
		.def("scaleBy", &AcGePoint2d::scaleBy,
			py::arg("scale"), py::arg("wrtPoint") = AcGePoint2d::kOrigin)

		// Scalar operations
		.def(py::self * double())
		.def(double() * py::self)
		.def(py::self *= double())
		.def(py::self / double())
		.def(py::self /= double())

		// Vector operations
		.def("__add__", [](const AcGePoint2d& p, const AcGeVector2d& v) {
		return p + v;
			}, py::arg("vec"))
		.def("__iadd__", [](AcGePoint2d& p, const AcGeVector2d& v) -> AcGePoint2d& {
		return p += v;
			}, py::arg("vec"))
		.def("__sub__", py::overload_cast<const AcGeVector2d&>(&AcGePoint2d::operator-, py::const_),
			py::arg("vec"))
		// [ADD] in-place vector subtraction (SDK operator -= )
		.def("__isub__", [](AcGePoint2d& p, const AcGeVector2d& v) -> AcGePoint2d& {
		return p -= v;
			}, py::arg("vec"))
		.def("setToSum", &AcGePoint2d::setToSum,
			py::arg("pnt"), py::arg("vec"))
		.def("setToProduct", &AcGePoint2d::setToProduct,
			py::arg("mat"), py::arg("pnt"))

		// Point-to-point operations
		.def("__sub__", py::overload_cast<const AcGePoint2d&>(&AcGePoint2d::operator-, py::const_),
			py::arg("pnt"))
		.def("asVector", &AcGePoint2d::asVector)

		// Geometric calculations
		.def("distanceTo", &AcGePoint2d::distanceTo,
			py::arg("pnt"))

		// Comparison operations
		.def("__eq__", &AcGePoint2d::operator==, py::arg("pnt"))
		.def("__ne__", &AcGePoint2d::operator!=, py::arg("pnt"))
		.def("isEqualTo", &AcGePoint2d::isEqualTo,
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)

		// Data access
		.def("__getitem__", [](const AcGePoint2d& p, int i) {
		return p[i];
			})
		.def("__setitem__", [](AcGePoint2d& p, int i, double val) {
		p[i] = val;
			})
		.def("set", &AcGePoint2d::set,
			py::arg("x"), py::arg("y"))

		// Member variables
		.def_readwrite("x", &AcGePoint2d::x)
		.def_readwrite("y", &AcGePoint2d::y)

		// String representation
		.def("__repr__", [](const AcGePoint2d& p) {
		return std::format("AcGePoint2d({:.3f}, {:.3f})", p.x, p.y);
			});
}
