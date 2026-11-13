#include "stdafx.h"
#include "PyGeLineSeg3d.h"
#include <format>
#include <string>

void bind_AcGeLineSeg3d(pybind11::module& m) {
	py::class_<AcGeLineSeg3d, AcGeLinearEnt3d>(m, "AcGeLineSeg3d")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, const AcGePoint3d&>(),
			py::arg("startPoint"), py::arg("endPoint"))
		.def(py::init<const AcGePoint3d&, const AcGeVector3d&>(),
			py::arg("pnt"), py::arg("vec"))
		.def("startPoint", &AcGeLineSeg3d::startPoint)
		.def("endPoint", &AcGeLineSeg3d::endPoint)
		.def("midPoint", &AcGeLineSeg3d::midPoint)
		.def("length", py::overload_cast<>(&AcGeLineSeg3d::length, py::const_))
		.def("direction", &AcGeLineSeg3d::direction)
		.def("set", py::overload_cast<const AcGePoint3d&, const AcGeVector3d&>(&AcGeLineSeg3d::set), py::arg("pnt"), py::arg("vec"))
		.def("set", py::overload_cast<const AcGePoint3d&, const AcGePoint3d&>(&AcGeLineSeg3d::set), py::arg("pnt1"), py::arg("pnt2"))
		.def("baryComb", &AcGeLineSeg3d::baryComb, py::arg("blendCoeff"))
		.def("__repr__", [](const AcGeLineSeg3d& s) {
			AcGePoint3d a = s.startPoint(), b = s.endPoint();
			return std::format("AcGeLineSeg3d(({:.3f},{:.3f},{:.3f})->({:.3f},{:.3f},{:.3f}), len={:.3f})",
				a.x, a.y, a.z, b.x, b.y, b.z, s.length());
			});
}
