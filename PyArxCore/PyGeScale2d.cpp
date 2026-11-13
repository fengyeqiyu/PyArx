#include "stdafx.h"
#include "PyGeScale2d.h"
#include <gemat2d.h>
#include <format>
#include <string>

void bind_AcGeScale2d(pybind11::module& m) {
	py::class_<AcGeScale2d>(m, "AcGeScale2d")
		// ---- 构造 ----
		.def(py::init<>())
		.def(py::init<const AcGeScale2d&>(), py::arg("src"), "拷贝构造")
		.def(py::init<double>(), py::arg("factor"), "均匀缩放构造")
		.def(py::init<double, double>(), py::arg("sx"), py::arg("sy"))
		.def_readonly_static("kIdentity", &AcGeScale2d::kIdentity)

		// ---- 分量 ----
		.def_readwrite("sx", &AcGeScale2d::sx)
		.def_readwrite("sy", &AcGeScale2d::sy)
		.def("set", &AcGeScale2d::set, py::arg("sc0"), py::arg("sc1"))
		.def("__getitem__", [](const AcGeScale2d& self, int i) -> double {
			if (i < 0 || i > 1) {
				acutPrintf(L"[PyArx] AcGeScale2d::__getitem__ 索引越界 (%d/2)\n", i);
				return 0.0;
			}
			return self[static_cast<unsigned int>(i)];
		}, py::arg("index"), "按索引读取分量（0=sx, 1=sy）")
		.def("__setitem__", [](AcGeScale2d& self, int i, double v) -> bool {
			if (i < 0 || i > 1) {
				acutPrintf(L"[PyArx] AcGeScale2d::__setitem__ 索引越界 (%d/2)\n", i);
				return false;
			}
			self[static_cast<unsigned int>(i)] = v;
			return true;
		}, py::arg("index"), py::arg("value"), "按索引设置分量；成功返回 true")

		// ---- 乘法 ----
		.def(py::self * py::self)
		.def(py::self * double())
		.def(double() * py::self)
		.def(py::self *= py::self)
		.def(py::self *= double())
		.def("preMultBy", [](AcGeScale2d& self, const AcGeScale2d& left) -> AcGeScale2d& {
			return self.preMultBy(left);
		}, py::arg("left_side"), py::return_value_policy::reference)
		.def("postMultBy", [](AcGeScale2d& self, const AcGeScale2d& right) -> AcGeScale2d& {
			return self.postMultBy(right);
		}, py::arg("right_side"), py::return_value_policy::reference)
		.def("setToProduct", [](AcGeScale2d& self, const AcGeScale2d& a, const AcGeScale2d& b)
				-> AcGeScale2d& {
			return self.setToProduct(a, b);
		}, py::arg("scale_a"), py::arg("scale_b"), py::return_value_policy::reference)
		.def("setToProduct", [](AcGeScale2d& self, const AcGeScale2d& a, double s)
				-> AcGeScale2d& {
			return self.setToProduct(a, s);
		}, py::arg("scale_a"), py::arg("factor"), py::return_value_policy::reference)

		// ---- 逆 ----
		.def("inverse", &AcGeScale2d::inverse)
		.def("isProportional", &AcGeScale2d::isProportional,
			py::arg("tol") = AcGeContext::gTol)
		.def("invert", &AcGeScale2d::invert)

		// ---- 比较 ----
		.def("isEqualTo", &AcGeScale2d::isEqualTo,
			py::arg("other"), py::arg("tol") = AcGeContext::gTol)
		.def("__eq__", &AcGeScale2d::operator==, py::arg("other"))
		.def("__ne__", &AcGeScale2d::operator!=, py::arg("other"))

		// ---- 与矩阵互转 ----
		.def("getMatrix", [](const AcGeScale2d& self) -> AcGeMatrix2d {
			AcGeMatrix2d mat;
			self.getMatrix(mat);
			return mat;
		}, "转换为对应的 2D 缩放矩阵")
		.def("extractScale", [](AcGeScale2d& self, const AcGeMatrix2d& mat)
				-> AcGeScale2d& {
			return self.extractScale(mat);
		}, py::arg("matrix"), py::return_value_policy::reference,
			"从矩阵中提取缩放分量")
		.def("removeScale", [](AcGeScale2d& self, AcGeMatrix2d& mat)
				-> AcGeMatrix2d& {
			self.removeScale(mat);
			return mat;
		}, py::arg("matrix"), py::return_value_policy::reference,
			"从矩阵中移除缩放分量（矩阵被原地修改）")

		.def("__repr__", [](const AcGeScale2d& s) {
			return std::format("AcGeScale2d({:.3f},{:.3f})", s.sx, s.sy);
			});
}
