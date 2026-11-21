#include "stdafx.h"
#include "PyGeScale3d.h"
#include <gemat3d.h>
#include <format>
#include <string>

void bind_AcGeScale3d(pybind11::module& m) {
	py::class_<AcGeScale3d>(m, "AcGeScale3d")
		// ---- 构造 ----
		.def(py::init<>())
		.def(py::init<const AcGeScale3d&>(), py::arg("src"), "拷贝构造")
		.def(py::init<double>(), py::arg("factor"), "均匀缩放构造")
		.def(py::init<double, double, double>(),
			py::arg("sx"), py::arg("sy"), py::arg("sz"))
		.def_readonly_static("kIdentity", &AcGeScale3d::kIdentity)

		// ---- 分量 ----
		.def_readwrite("sx", &AcGeScale3d::sx)
		.def_readwrite("sy", &AcGeScale3d::sy)
		.def_readwrite("sz", &AcGeScale3d::sz)
		.def("set", &AcGeScale3d::set,
			py::arg("sc0"), py::arg("sc1"), py::arg("sc2"))
		.def("__getitem__", [](const AcGeScale3d& self, int i) -> double {
			if (i < 0 || i > 2) {
				acutPrintf(L"[PyArx] AcGeScale3d::__getitem__ 索引越界 (%d/3)\n", i);
				return 0.0;
			}
			return self[static_cast<unsigned int>(i)];
		}, py::arg("index"), "按索引读取分量（0=sx, 1=sy, 2=sz）")
		.def("__setitem__", [](AcGeScale3d& self, int i, double v) -> bool {
			if (i < 0 || i > 2) {
				acutPrintf(L"[PyArx] AcGeScale3d::__setitem__ 索引越界 (%d/3)\n", i);
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
		.def("preMultBy", [](AcGeScale3d& self, const AcGeScale3d& left) -> AcGeScale3d& {
			return self.preMultBy(left);
		}, py::arg("left_side"), py::return_value_policy::reference)
		.def("postMultBy", [](AcGeScale3d& self, const AcGeScale3d& right) -> AcGeScale3d& {
			return self.postMultBy(right);
		}, py::arg("right_side"), py::return_value_policy::reference)
		.def("setToProduct", [](AcGeScale3d& self, const AcGeScale3d& a, const AcGeScale3d& b)
				-> AcGeScale3d& {
			return self.setToProduct(a, b);
		}, py::arg("scale_a"), py::arg("scale_b"), py::return_value_policy::reference)
		.def("setToProduct", [](AcGeScale3d& self, const AcGeScale3d& a, double s)
				-> AcGeScale3d& {
			return self.setToProduct(a, s);
		}, py::arg("scale_a"), py::arg("factor"), py::return_value_policy::reference)

		// ---- 逆 ----
		.def("inverse", &AcGeScale3d::inverse)
		.def("isProportional", &AcGeScale3d::isProportional,
			py::arg("tol") = AcGeContext::gTol)
		.def("invert", &AcGeScale3d::invert)

		// ---- 比较 ----
		.def("isEqualTo", &AcGeScale3d::isEqualTo,
			py::arg("other"), py::arg("tol") = AcGeContext::gTol)
		.def("__eq__", &AcGeScale3d::operator==, py::arg("other"))
		.def("__ne__", &AcGeScale3d::operator!=, py::arg("other"))

		// ---- 与矩阵互转 ----
		.def("getMatrix", [](const AcGeScale3d& self) -> AcGeMatrix3d {
			AcGeMatrix3d mat;
			self.getMatrix(mat);
			return mat;
		}, "转换为对应的 3D 缩放矩阵")
		.def("extractScale", [](AcGeScale3d& self, const AcGeMatrix3d& mat)
				-> AcGeScale3d& {
			return self.extractScale(mat);
		}, py::arg("matrix"), py::return_value_policy::reference,
			"从矩阵中提取缩放分量")
		.def("removeScale", [](AcGeScale3d& self, AcGeMatrix3d& mat)
				-> AcGeMatrix3d& {
			self.removeScale(mat);
			return mat;
		}, py::arg("matrix"), py::return_value_policy::reference,
			"从矩阵中移除缩放分量（矩阵被原地修改）")

		.def("__repr__", [](const AcGeScale3d& s) {
			return std::format("AcGeScale3d({:.3f},{:.3f},{:.3f})", s.sx, s.sy, s.sz);
			});
}
