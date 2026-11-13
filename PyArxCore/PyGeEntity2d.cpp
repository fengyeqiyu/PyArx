#include "stdafx.h"
#include "PyGeEntity2d.h"
#include <format>
#include <string>

void bind_AcGeEntity2d(pybind11::module& m) {
	// Abstract base: no py::init.
	py::class_<AcGeEntity2d>(m, "AcGeEntity2d")
		// Runtime type information (AcGe::EntityId exposed as plain int)
		.def("isKindOf", [](const AcGeEntity2d& self, int entType) {
			return self.isKindOf(static_cast<AcGe::EntityId>(entType));
		}, py::arg("entType"), "Return True if this entity is of the given EntityId type or a derived type")
		.def("type", [](const AcGeEntity2d& self) -> int {
			return static_cast<int>(self.type());
		}, "Return the AcGe::EntityId runtime type identifier of this entity")
		// Deep copy; the returned object is owned by the Python wrapper
		.def("copy", [](const AcGeEntity2d& self) {
			return std::unique_ptr<AcGeEntity2d>(self.copy());
		}, "Make a deep, independently owned copy of this entity")
		.def("isOn",
			py::overload_cast<const AcGePoint2d&, const AcGeTol&>(&AcGeEntity2d::isOn, py::const_),
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("isEqualTo", &AcGeEntity2d::isEqualTo,
			py::arg("other"), py::arg("tol") = AcGeContext::gTol)
		.def("transformBy", &AcGeEntity2d::transformBy, py::arg("xfm"))
		.def("translateBy", &AcGeEntity2d::translateBy, py::arg("vec"))
		.def("rotateBy", &AcGeEntity2d::rotateBy, py::arg("angle"), py::arg("wrtPoint") = AcGePoint2d::kOrigin)
		.def("mirror", &AcGeEntity2d::mirror, py::arg("line"))
		.def("scaleBy", &AcGeEntity2d::scaleBy, py::arg("factor"), py::arg("wrtPoint") = AcGePoint2d::kOrigin)
		.def("__eq__", &AcGeEntity2d::operator==, py::arg("other"))
		.def("__ne__", &AcGeEntity2d::operator!=, py::arg("other"))
		.def("__repr__", [](const AcGeEntity2d&) {
			return AcString(L"AcGeEntity2d");
			});
}
