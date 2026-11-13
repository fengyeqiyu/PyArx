#include "stdafx.h"
#include "PyGeKnotVector.h"
#include <format>
#include <string>

void bind_AcGeKnotVector(pybind11::module& m) {
	py::class_<AcGeKnotVector>(m, "AcGeKnotVector", "样条节点向量（NURB 用）")
		// ---- 构造 ----
		.def(py::init<>(), "默认构造")
		.def(py::init<double>(), py::arg("eps"), "按容差构造")
		.def(py::init<int, int, double>(), py::arg("size"), py::arg("grow_size"),
			py::arg("eps") = AcGeKnotVector::globalKnotTolerance, "按长度/增长量/容差构造")
		.def(py::init([](const std::vector<double>& vals, double eps) {
			// 用 (size, const double[], eps) 构造：直接带数据并设逻辑长度
			AcGeKnotVector kv(static_cast<int>(vals.size()), vals.data(), eps);
			return kv;
		}), py::arg("values"), py::arg("eps") = AcGeKnotVector::globalKnotTolerance,
			"按 double 列表构造")
		.def(py::init<const AcGeKnotVector&>(), py::arg("src"), "拷贝构造")
		.def(py::init([](int plusMult, const AcGeKnotVector& src) {
			return AcGeKnotVector(plusMult, src);
		}), py::arg("plus_mult"), py::arg("src"),
			"按源向量构造，并把每个不同节点的重数提升 plus_mult")
		// ---- 静态容差 ----
		.def_readwrite_static("globalKnotTolerance", &AcGeKnotVector::globalKnotTolerance,
			"全局节点容差（默认 1.0e-9）")

		// ---- 索引 ----
		.def("at", [](const AcGeKnotVector& self, int i) -> double {
			if (i < 0 || i >= self.length()) {
				acutPrintf(L"[PyArx] AcGeKnotVector::at 索引越界 (%d/%d)\n", i, self.length());
				return 0.0;
			}
			return self[i];
		}, py::arg("index"), "按索引读取节点值（越界返回 0 并提示）")
		.def("set", [](AcGeKnotVector& self, int i, double v) -> bool {
			if (i < 0 || i >= self.length()) {
				acutPrintf(L"[PyArx] AcGeKnotVector::set 索引越界 (%d/%d)\n", i, self.length());
				return false;
			}
			self[i] = v;
			return true;
		}, py::arg("index"), py::arg("value"), "按索引设置节点值；成功返回 true")

		// ---- 增删 ----
		.def("append", [](AcGeKnotVector& self, double v) -> int { return self.append(v); },
			py::arg("value"), "末尾追加节点，返回新逻辑长度")
		.def("append", [](AcGeKnotVector& self, AcGeKnotVector& tail, double knotRatio)
				-> AcGeKnotVector& {
			return self.append(tail, knotRatio);
		}, py::arg("tail"), py::arg("knot_ratio") = 0.0,
			py::return_value_policy::reference, "把另一节点向量尾部拼接到本向量")
		.def("insertAt", [](AcGeKnotVector& self, int idx, double u, int mult) {
			self.insertAt(idx, u, mult);
		}, py::arg("index"), py::arg("u"), py::arg("multiplicity") = 1, "在指定位置插入节点（可带重数）")
		.def("insert", [](AcGeKnotVector& self, double u) {
			self.insert(u);
		}, py::arg("u"), "插入节点")
		.def("removeAt", [](AcGeKnotVector& self, int idx) {
			self.removeAt(idx);
		}, py::arg("index"), "删除指定位置节点")
		.def("removeSubVector", [](AcGeKnotVector& self, int start, int end) {
			self.removeSubVector(start, end);
		}, py::arg("start_index"), py::arg("end_index"), "删除闭区间 [start,end] 的节点")
		.def("reverse", [](AcGeKnotVector& self) {
			self.reverse();
		}, "翻转节点向量")

		// ---- 查询 ----
		.def("length", &AcGeKnotVector::length, "逻辑长度（节点数）")
		.def("isEmpty", &AcGeKnotVector::isEmpty, "是否为空")
		.def("logicalLength", &AcGeKnotVector::logicalLength, "逻辑长度")
		.def("physicalLength", &AcGeKnotVector::physicalLength, "物理长度")
		.def("startParam", &AcGeKnotVector::startParam, "起始参数")
		.def("endParam", &AcGeKnotVector::endParam, "终止参数")
		.def("multiplicityAt", [](const AcGeKnotVector& self, int i) -> int {
			if (i < 0 || i >= self.length()) {
				acutPrintf(L"[PyArx] AcGeKnotVector::multiplicityAt 索引越界 (%d/%d)\n",
					i, self.length());
				return 0;
			}
			return self.multiplicityAt(i);
		}, py::arg("index"), "指定索引处节点的重数")
		.def("multiplicityAt", [](const AcGeKnotVector& self, double par) -> int {
			return self.multiplicityAt(par);
		}, py::arg("param"), "指定参数处的重数")
		.def("numIntervals", &AcGeKnotVector::numIntervals, "区间数")
		.def("contains", [](const AcGeKnotVector& self, double par) -> bool {
			return self.contains(par) != Adesk::kFalse;
		}, py::arg("param"), "是否包含参数")
		.def("isOn", [](const AcGeKnotVector& self, double knot) -> bool {
			return self.isOn(knot) != Adesk::kFalse;
		}, py::arg("knot"), "knot 是否恰为某节点")
		.def("isEqualTo", [](const AcGeKnotVector& self, const AcGeKnotVector& other) -> bool {
			return self.isEqualTo(other) != Adesk::kFalse;
		}, py::arg("other"), "与另一节点向量是否相等")
		.def("getDistinctKnots", [](const AcGeKnotVector& self) -> std::vector<double> {
			AcGeDoubleArray knots;
			self.getDistinctKnots(knots);
			std::vector<double> out;
			out.reserve(knots.length());
			for (int i = 0; i < knots.length(); ++i) out.push_back(knots[i]);
			return out;
		}, "返回去重后的不同节点值列表")
		.def("toList", [](const AcGeKnotVector& self) -> std::vector<double> {
			std::vector<double> out;
			out.reserve(self.length());
			for (int i = 0; i < self.length(); ++i) out.push_back(self[i]);
			return out;
		}, "返回全部节点值列表")

		// ---- 区间 / 容差 / 长度 ----
		.def("getInterval", [](const AcGeKnotVector& self, int ord, double par) -> py::tuple {
			AcGeInterval ivl;
			int rc = self.getInterval(ord, par, ivl);
			return py::make_tuple(rc, ivl.lowerBound(), ivl.upperBound());
		}, py::arg("ord"), py::arg("par"),
			"返回 (返回码, 区间下界, 区间上界)")
		.def("setRange", [](AcGeKnotVector& self, double lower, double upper) {
			self.setRange(lower, upper);
		}, py::arg("lower"), py::arg("upper"), "重设参数范围")
		.def("tolerance", &AcGeKnotVector::tolerance, "当前容差")
		.def("setTolerance", [](AcGeKnotVector& self, double tol) {
			self.setTolerance(tol);
		}, py::arg("tol"), "设置容差")
		.def("set", [](AcGeKnotVector& self, const std::vector<double>& vals, double eps)
				-> AcGeKnotVector& {
			return self.set(static_cast<int>(vals.size()),
				vals.empty() ? nullptr : vals.data(), eps);
		}, py::arg("values"), py::arg("eps") = AcGeKnotVector::globalKnotTolerance,
			py::return_value_policy::reference, "用一组节点值重设节点向量")
		.def("setLogicalLength", [](AcGeKnotVector& self, int len) {
			self.setLogicalLength(len);
		}, py::arg("length"), "设置逻辑长度")
		.def("setPhysicalLength", [](AcGeKnotVector& self, int len) {
			self.setPhysicalLength(len);
		}, py::arg("length"), "设置物理长度")
		.def("growLength", &AcGeKnotVector::growLength, "增长长度")
		.def("setGrowLength", [](AcGeKnotVector& self, int g) {
			self.setGrowLength(g);
		}, py::arg("grow_length"), "设置增长长度")

		// ---- 字符串 ----
		.def("__len__", &AcGeKnotVector::length)
		.def("__repr__", [](const AcGeKnotVector& self) {
			return std::format("AcGeKnotVector(len={}, start={:.3f}, end={:.3f})",
				self.length(), self.startParam(), self.endParam());
		})
		.def("__str__", [](const AcGeKnotVector& self) {
			std::string s = "[";
			for (int i = 0; i < self.length(); ++i) {
				if (i) s += ", ";
				s += std::format("{:.6g}", self[i]);
			}
			s += "]";
				return AcString(s.c_str(), AcString::Utf8);
		});
}
