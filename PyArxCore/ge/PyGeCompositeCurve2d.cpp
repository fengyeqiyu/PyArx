#include "stdafx.h"
#include "PyGeCompositeCurve2d.h"
#include <format>
#include <string>

void bind_AcGeCompositeCurve2d(pybind11::module& m) {
	py::class_<AcGeCompositeCurve2d, AcGeCurve2d>(m, "AcGeCompositeCurve2d")
		.def(py::init([](py::list curves) {
				AcGeVoidPointerArray arr;
				for (auto item : curves) arr.append(item.cast<AcGeCurve2d*>());
				return new AcGeCompositeCurve2d(arr);
				}), py::arg("curves"), "Construct from a list of AcGeCurve2d (borrowed; keep curves alive while composite lives)")
			.def(py::init<>())
			.def("globalToLocalParam", [](const AcGeCompositeCurve2d& self, double param) {
				int segNum = 0;
				double local = self.globalToLocalParam(param, segNum);
				return py::make_tuple(local, segNum);
				}, py::arg("param"), "Return (localParam, segmentIndex)")
			.def("localToGlobalParam", &AcGeCompositeCurve2d::localToGlobalParam,
				py::arg("param"), py::arg("segment"))
			.def("getCurveList", [](const AcGeCompositeCurve2d& self) {
				AcGeVoidPointerArray arr;
				self.getCurveList(arr);
				py::list out;
				for (int i = 0; i < (int)arr.length(); ++i)
					out.append(reinterpret_cast<AcGeCurve2d*>(arr[i]));
				return out;
				}, py::return_value_policy::reference, "Return the component curves (borrowed references)")
			.def("numCurves", [](const AcGeCompositeCurve2d& self) {
				AcGeVoidPointerArray arr;
				self.getCurveList(arr);
				return (int)arr.length();
				})
			.def("setCurveList", [](AcGeCompositeCurve2d& self, py::list curves) {
				AcGeVoidPointerArray arr;
				for (auto item : curves)
					arr.append(item.cast<AcGeCurve2d*>());
				self.setCurveList(arr);
				return self;
				}, py::arg("curves"), py::return_value_policy::reference)
			.def("__repr__", [](const AcGeCompositeCurve2d&) {
			return AcString(L"AcGeCompositeCurve2d");
			});
}
