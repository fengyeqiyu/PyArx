#include "stdafx.h"
#include "PyGeInterval.h"
#include <format>
#include <string>

void bind_AcGeInterval(pybind11::module& m) {
	py::class_<AcGeInterval>(m, "AcGeInterval", "参数区间")
		// ---- 构造 ----
		.def(py::init<>(), "默认构造")
		.def(py::init<double>(), py::arg("tol"), "按容差构造")
		.def(py::init<double, double, double>(), py::arg("lower"), py::arg("upper"),
			py::arg("tol") = 1.e-12, "按下界/上界/容差构造")
		.def(py::init([](bool boundedBelow, double bound, double tol) {
			return AcGeInterval(boundedBelow ? Adesk::kTrue : Adesk::kFalse, bound, tol);
		}), py::arg("bounded_below"), py::arg("bound"), py::arg("tol") = 1.e-12,
			"按单边约束构造（bounded_below=true 表示下界约束）")
		.def(py::init<const AcGeInterval&>(), py::arg("src"), "拷贝构造")

		// ---- 查询 ----
		.def("lowerBound", &AcGeInterval::lowerBound, "下界（无下界时行为见 SDK）")
		.def("upperBound", &AcGeInterval::upperBound, "上界（无上界时行为见 SDK）")
		.def("element", &AcGeInterval::element, "单点区间的元素值")
		.def("getBounds", [](const AcGeInterval& self) -> std::pair<double, double> {
			double lo = 0, hi = 0;
			self.getBounds(lo, hi);
			return { lo, hi };
		}, "返回 (下界, 上界)")
		.def("length", &AcGeInterval::length, "区间长度")
		.def("tolerance", &AcGeInterval::tolerance, "容差")

		// ---- 设置 ----
		.def("setBounds", [](AcGeInterval& self, double lower, double upper) {
			self.set(lower, upper);
		}, py::arg("lower"), py::arg("upper"), "设置下界/上界")
		.def("setBounded", [](AcGeInterval& self, bool boundedBelow, double bound) {
			self.set(boundedBelow ? Adesk::kTrue : Adesk::kFalse, bound);
		}, py::arg("bounded_below"), py::arg("bound"), "按单边约束设置")
		.def("setEmpty", [](AcGeInterval& self) { self.set(); }, "设为空区间")
		.def("setUpper", &AcGeInterval::setUpper, py::arg("upper"), "设置上界")
		.def("setLower", &AcGeInterval::setLower, py::arg("lower"), "设置下界")
		.def("setTolerance", &AcGeInterval::setTolerance, py::arg("tol"), "设置容差")

		// ---- 集合运算 ----
		.def("getMerge", [](const AcGeInterval& self, const AcGeInterval& other) -> py::tuple {
			AcGeInterval res;
			self.getMerge(other, res);
			return py::make_tuple(res.lowerBound(), res.upperBound());
		}, py::arg("other"), "合并区间，返回 (下界, 上界)")
		.def("subtract", [](const AcGeInterval& self, const AcGeInterval& other) -> py::tuple {
			AcGeInterval l, r;
			int rc = self.subtract(other, l, r);
			// 返回码语义：0=结果为空,1=l 有效,2=r 有效,3=两者有效
			return py::make_tuple(rc, l.lowerBound(), l.upperBound(), r.lowerBound(), r.upperBound());
		}, py::arg("other"), "减去另一区间，返回 (码, l低, l高, r低, r高)")
		.def("intersectWith", [](const AcGeInterval& self, const AcGeInterval& other) -> py::tuple {
			AcGeInterval res;
			bool ok = self.intersectWith(other, res) != Adesk::kFalse;
			return py::make_tuple(ok, res.lowerBound(), res.upperBound());
		}, py::arg("other"), "求交集，返回 (是否有交, 交下界, 交上界)")

		// ---- 关系 ----
		.def("isBounded", [](const AcGeInterval& self) -> bool {
			return self.isBounded() != Adesk::kFalse;
		}, "是否完全有界")
		.def("isBoundedAbove", [](const AcGeInterval& self) -> bool {
			return self.isBoundedAbove() != Adesk::kFalse;
		}, "是否有上界")
		.def("isBoundedBelow", [](const AcGeInterval& self) -> bool {
			return self.isBoundedBelow() != Adesk::kFalse;
		}, "是否有下界")
		.def("isUnBounded", [](const AcGeInterval& self) -> bool {
			return self.isUnBounded() != Adesk::kFalse;
		}, "是否完全无界")
		.def("isSingleton", [](const AcGeInterval& self) -> bool {
			return self.isSingleton() != Adesk::kFalse;
		}, "是否单点区间")
		.def("isDisjoint", [](const AcGeInterval& self, const AcGeInterval& other) -> bool {
			return self.isDisjoint(other) != Adesk::kFalse;
		}, py::arg("other"), "是否与另一区间不相交")
		.def("containsInterval", [](const AcGeInterval& self, const AcGeInterval& other) -> bool {
			return self.contains(other) != Adesk::kFalse;
		}, py::arg("other"), "是否包含另一区间")
		.def("containsValue", [](const AcGeInterval& self, double v) -> bool {
			return self.contains(v) != Adesk::kFalse;
		}, py::arg("value"), "是否包含值")
		.def("isContinuousAtUpper", [](const AcGeInterval& self, const AcGeInterval& other) -> bool {
			return self.isContinuousAtUpper(other) != Adesk::kFalse;
		}, py::arg("other"), "上端是否与另一区间连续")
		.def("isOverlapAtUpper", [](const AcGeInterval& self, const AcGeInterval& other) -> py::tuple {
			AcGeInterval overlap;
			bool ok = self.isOverlapAtUpper(other, overlap) != Adesk::kFalse;
			return py::make_tuple(ok, overlap.lowerBound(), overlap.upperBound());
		}, py::arg("other"), "上端是否与另一区间重叠，返回 (是否有, 重叠低, 重叠高)")

		// ---- 比较 ----
		.def("__eq__", [](const AcGeInterval& self, const AcGeInterval& other) -> bool {
			return self == other;
		}, py::arg("other"))
		.def("__ne__", [](const AcGeInterval& self, const AcGeInterval& other) -> bool {
			return self != other;
		}, py::arg("other"))

		// ---- 端点相等判定 ----
		.def("isEqualAtUpper", [](const AcGeInterval& self, const AcGeInterval& other) -> bool {
			return self.isEqualAtUpper(other) != Adesk::kFalse;
		}, py::arg("other"), "上界是否与另一区间上界相等")
		.def("isEqualAtUpper", [](const AcGeInterval& self, double v) -> bool {
			return self.isEqualAtUpper(v) != Adesk::kFalse;
		}, py::arg("value"), "上界是否等于给定值")
		.def("isEqualAtLower", [](const AcGeInterval& self, const AcGeInterval& other) -> bool {
			return self.isEqualAtLower(other) != Adesk::kFalse;
		}, py::arg("other"), "下界是否与另一区间下界相等")
		.def("isEqualAtLower", [](const AcGeInterval& self, double v) -> bool {
			return self.isEqualAtLower(v) != Adesk::kFalse;
		}, py::arg("value"), "下界是否等于给定值")

		// ---- 周期曲线用 ----
		.def("isPeriodicallyOn", [](AcGeInterval& self, double period) -> py::tuple {
			double val = 0.0;
			bool ok = self.isPeriodicallyOn(period, val) != Adesk::kFalse;
			return py::make_tuple(ok, val);
		}, py::arg("period"), "周期参数是否落在区间上，返回 (是否命中, 映射值)")
		.def("__repr__", [](const AcGeInterval& self) {
			return std::format("AcGeInterval(lower={:.6g}, upper={:.6g}, tol={:.2e})",
				self.lowerBound(), self.upperBound(), self.tolerance());
		});
}
