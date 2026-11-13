#include "stdafx.h"
#include "PyGeEntity3d.h"
#include <format>
#include <string>

void bind_AcGeEntity3d(pybind11::module& m) {
	// Abstract base: no py::init.
	py::class_<AcGeEntity3d>(m, "AcGeEntity3d")
		// Runtime type information (AcGe::EntityId exposed as plain int)
		.def("isKindOf", [](const AcGeEntity3d& self, int entType) {
			return self.isKindOf(static_cast<AcGe::EntityId>(entType));
		}, py::arg("entType"), "Return True if this entity is of the given EntityId type or a derived type")
		.def("type", [](const AcGeEntity3d& self) -> int {
			return static_cast<int>(self.type());
		}, "Return the AcGe::EntityId runtime type identifier of this entity")
		// Deep copy; the returned object is owned by the Python wrapper
		.def("copy", [](const AcGeEntity3d& self) {
			return std::unique_ptr<AcGeEntity3d>(self.copy());
		}, "Make a deep, independently owned copy of this entity")
		.def("isOn",
			py::overload_cast<const AcGePoint3d&, const AcGeTol&>(&AcGeEntity3d::isOn, py::const_),
			py::arg("pnt"), py::arg("tol") = AcGeContext::gTol)
		.def("isEqualTo", &AcGeEntity3d::isEqualTo,
			py::arg("other"), py::arg("tol") = AcGeContext::gTol)
		.def("transformBy", &AcGeEntity3d::transformBy, py::arg("xfm"))
		.def("translateBy", &AcGeEntity3d::translateBy, py::arg("vec"))
		.def("rotateBy", &AcGeEntity3d::rotateBy, py::arg("angle"), py::arg("axis"), py::arg("wrtPoint") = AcGePoint3d::kOrigin)
		.def("mirror", &AcGeEntity3d::mirror, py::arg("plane"))
		.def("scaleBy", &AcGeEntity3d::scaleBy, py::arg("factor"), py::arg("wrtPoint") = AcGePoint3d::kOrigin)
		.def("__eq__", &AcGeEntity3d::operator==, py::arg("other"))
		.def("__ne__", &AcGeEntity3d::operator!=, py::arg("other"))
		.def("__repr__", [](const AcGeEntity3d&) {
			return AcString(L"AcGeEntity3d");
			});
}
