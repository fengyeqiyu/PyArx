#include "stdafx.h"
#include "PyGeTol.h"

#include <format>
#include <string>

void bind_AcGeTol(pybind11::module& m) {
	py::class_<AcGeTol>(m, "AcGeTol")
		// Constructors
		.def(py::init<>())
		// Property accessors
		.def_property("equalPoint",
			&AcGeTol::equalPoint,
			&AcGeTol::setEqualPoint)

		.def_property("equalVector",
			&AcGeTol::equalVector,
			&AcGeTol::setEqualVector)

		// Comparison operators
		.def("__eq__", [](const AcGeTol& self, const AcGeTol& other) {
		return self.equalPoint() == other.equalPoint() &&
			self.equalVector() == other.equalVector();
			}, py::arg("other"))

		// String representation
		.def("__repr__", [](const AcGeTol& self) {
		return std::format("AcGeTol(point_tol={:.3f}, vector_tol={:.3f})",
			self.equalPoint(),
			self.equalVector());
			})
		// Static methods
		.def_static("gTol", []() -> AcGeTol& { return AcGeContext::gTol; }, py::return_value_policy::reference);
}
