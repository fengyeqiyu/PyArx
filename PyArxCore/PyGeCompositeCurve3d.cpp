#include "stdafx.h"
#include "PyGeCompositeCurve3d.h"
#include <format>
#include <string>

void bind_AcGeCompositeCurve3d(pybind11::module& m) {
	py::class_<AcGeCompositeCurve3d, AcGeCurve3d>(m, "AcGeCompositeCurve3d")
		.def(py::init([](py::list curves) {
				AcGeVoidPointerArray arr;
				for (auto item : curves) arr.append(item.cast<AcGeCurve3d*>());
				return new AcGeCompositeCurve3d(arr);
				}), py::arg("curves"), "Construct from a list of AcGeCurve3d (borrowed; keep curves alive while composite lives)")
			.def(py::init<>())
		.def("globalToLocalParam", [](const AcGeCompositeCurve3d& self, double param) {
			int segNum = 0;
			double local = self.globalToLocalParam(param, segNum);
			return py::make_tuple(local, segNum);
			}, py::arg("param"), "Return (localParam, segmentIndex)")
		.def("localToGlobalParam", &AcGeCompositeCurve3d::localToGlobalParam,
			py::arg("param"), py::arg("segment"))
		.def("getCurveList", [](const AcGeCompositeCurve3d& self) {
				AcGeVoidPointerArray arr;
				self.getCurveList(arr);
				py::list out;
				for (int i = 0; i < (int)arr.length(); ++i)
					out.append(reinterpret_cast<AcGeCurve3d*>(arr[i]));
				return out;
				}, py::return_value_policy::reference, "Return the component curves (borrowed references)")
			.def("numCurves", [](const AcGeCompositeCurve3d& self) {
				AcGeVoidPointerArray arr;
				self.getCurveList(arr);
				return (int)arr.length();
				})
			.def("setCurveList", [](AcGeCompositeCurve3d& self, py::list curves) {
				AcGeVoidPointerArray arr;
				for (auto item : curves)
					arr.append(item.cast<AcGeCurve3d*>());
				self.setCurveList(arr);
				return self;
				}, py::arg("curves"), py::return_value_policy::reference)
			.def("__repr__", [](const AcGeCompositeCurve3d&) {
			return AcString(L"AcGeCompositeCurve3d");
			});
}
