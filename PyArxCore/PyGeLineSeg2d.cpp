#include "stdafx.h"
#include "PyGeLineSeg2d.h"
#include <format>
#include <string>

void bind_AcGeLineSeg2d(pybind11::module& m) {
	py::class_<AcGeLineSeg2d, AcGeLinearEnt2d>(m, "AcGeLineSeg2d")
		.def(py::init<>())
		.def(py::init<const AcGePoint2d&, const AcGePoint2d&>(),
			py::arg("startPoint"), py::arg("endPoint"))
		.def(py::init<const AcGePoint2d&, const AcGeVector2d&>(),
			py::arg("pnt"), py::arg("vec"))
		.def("startPoint", &AcGeLineSeg2d::startPoint)
		.def("endPoint", &AcGeLineSeg2d::endPoint)
		.def("midPoint", &AcGeLineSeg2d::midPoint)
		.def("length", py::overload_cast<>(&AcGeLineSeg2d::length, py::const_))
		.def("direction", &AcGeLineSeg2d::direction)
		.def("set", py::overload_cast<const AcGePoint2d&, const AcGeVector2d&>(&AcGeLineSeg2d::set), py::arg("pnt"), py::arg("vec"))
		.def("set", py::overload_cast<const AcGePoint2d&, const AcGePoint2d&>(&AcGeLineSeg2d::set), py::arg("pnt1"), py::arg("pnt2"))
		.def("baryComb", &AcGeLineSeg2d::baryComb, py::arg("blendCoeff"))
		.def("__repr__", [](const AcGeLineSeg2d& s) {
			AcGePoint2d a = s.startPoint(), b = s.endPoint();
			return std::format("AcGeLineSeg2d(({:.3f},{:.3f})->({:.3f},{:.3f}), len={:.3f})",
				a.x, a.y, b.x, b.y, s.length());
			});
}
