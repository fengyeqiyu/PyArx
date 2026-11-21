#include "stdafx.h"
#include "PyGeMatrix2d.h"

#include <format>
#include <string>

void bind_AcGeMatrix2d(pybind11::module& m) {
	py::class_<AcGeMatrix2d>(m, "AcGeMatrix2d")
		// 构造函数和基本操作
		.def(py::init<>())
		.def(py::init<const AcGeMatrix2d&>(), py::arg("src"))
		.def_readonly_static("kIdentity", &AcGeMatrix2d::kIdentity)
		.def("setToIdentity", &AcGeMatrix2d::setToIdentity)

		// 矩阵运算
		.def(py::self * py::self)
		.def(py::self *= py::self)
		.def("preMultBy", &AcGeMatrix2d::preMultBy, py::arg("leftSide"))
		.def("postMultBy", &AcGeMatrix2d::postMultBy, py::arg("rightSide"))
		.def("setToProduct", &AcGeMatrix2d::setToProduct,
			py::arg("mat1"), py::arg("mat2"))

		// 矩阵特性
		.def("invert", &AcGeMatrix2d::invert)
		.def("inverse", &AcGeMatrix2d::inverse)
		.def("isSingular", &AcGeMatrix2d::isSingular,
			py::arg("tol") = AcGeContext::gTol)
		.def("transposeIt", &AcGeMatrix2d::transposeIt)
		.def("transpose", &AcGeMatrix2d::transpose)
		.def("det", &AcGeMatrix2d::det)
		.def("scale", &AcGeMatrix2d::scale)

		// 变换属性
		.def("setTranslation", &AcGeMatrix2d::setTranslation, py::arg("vec"))
		.def("translation",
			py::overload_cast<>(&AcGeMatrix2d::translation, py::const_),
			"Returns the translation vector component of this matrix")

		.def("isConformal", [](const AcGeMatrix2d& self) {
		double scale, angle;
		Adesk::Boolean isMirror;
		AcGeVector2d reflex;
		bool res = self.isConformal(scale, angle, isMirror, reflex);
		return py::make_tuple(res, scale, angle, isMirror, reflex);
			}, "Returns tuple of (success, scale, angle, isMirror, reflexVector)")

		// 坐标系操作
		.def("setCoordSystem", &AcGeMatrix2d::setCoordSystem,
			py::arg("origin"), py::arg("e0"), py::arg("e1"))
		.def("getCoordSystem", [](const AcGeMatrix2d& self) {
		AcGePoint2d origin;
		AcGeVector2d e0, e1;
		self.getCoordSystem(origin, e0, e1);
		return py::make_tuple(origin, e0, e1);
			})

		// 变换设置
		.def("setToTranslation", &AcGeMatrix2d::setToTranslation, py::arg("vec"))
		.def("setToRotation", &AcGeMatrix2d::setToRotation,
			py::arg("angle"), py::arg("center") = AcGePoint2d::kOrigin)
		.def("setToScaling", &AcGeMatrix2d::setToScaling,
			py::arg("scale"), py::arg("center") = AcGePoint2d::kOrigin)
		.def("setToMirroring", py::overload_cast<const AcGePoint2d&>(&AcGeMatrix2d::setToMirroring),
			py::arg("pnt"))
		.def("setToMirroring", py::overload_cast<const AcGeLine2d&>(&AcGeMatrix2d::setToMirroring),
			py::arg("line"))
		.def("setToAlignCoordSys", &AcGeMatrix2d::setToAlignCoordSys,
			py::arg("fromOrigin"), py::arg("fromE0"), py::arg("fromE1"),
			py::arg("toOrigin"), py::arg("toE0"), py::arg("toE1"))

		// 静态构造方法
		.def_static("createDisplacement", py::overload_cast<const AcGeVector2d&>(&AcGeMatrix2d::translation),
			py::arg("vector"),
			"Creates a translation matrix from 2D vector")

		.def_static("createRotation", &AcGeMatrix2d::rotation,
			py::arg("angle"), py::arg("center") = AcGePoint2d::kOrigin)
		.def_static("createScaling", &AcGeMatrix2d::scaling,
			py::arg("scale"), py::arg("center") = AcGePoint2d::kOrigin)
		.def_static("createMirroring", py::overload_cast<const AcGePoint2d&>(&AcGeMatrix2d::mirroring),
			py::arg("pnt"))
		.def_static("createMirroring", py::overload_cast<const AcGeLine2d&>(&AcGeMatrix2d::mirroring),
			py::arg("line"))
		.def_static("createAlignCoordSys", &AcGeMatrix2d::alignCoordSys,
			py::arg("fromOrigin"), py::arg("fromE0"), py::arg("fromE1"),
			py::arg("toOrigin"), py::arg("toE0"), py::arg("toE1"))

		// 矩阵访问
		.def("__call__", [](const AcGeMatrix2d& m, unsigned int r, unsigned int c) {
		if (r >= 3 || c >= 3) throw py::index_error();
		return m(r, c);
			}, py::arg("row"), py::arg("col"))
		// [ADD] tuple-indexed read access, mirroring __setitem__
		.def("__getitem__", [](const AcGeMatrix2d& m, py::tuple idx) {
		unsigned int r = idx[0].cast<unsigned int>();
		unsigned int c = idx[1].cast<unsigned int>();
		if (r >= 3 || c >= 3) throw py::index_error();
		return m(r, c);
			}, py::arg("row_col"))
		.def("__setitem__", [](AcGeMatrix2d& m, py::tuple idx, double val) {
		unsigned int r = idx[0].cast<unsigned int>();
		unsigned int c = idx[1].cast<unsigned int>();
		if (r >= 3 || c >= 3) throw py::index_error();
		m(r, c) = val;
			})

		// 比较运算
		.def(py::self == py::self)
		.def(py::self != py::self)
		.def("isEqualTo", &AcGeMatrix2d::isEqualTo,
			py::arg("mat"), py::arg("tol") = AcGeContext::gTol)

		// 特殊属性检查
		.def("isUniScaledOrtho", &AcGeMatrix2d::isUniScaledOrtho,
			py::arg("tol") = AcGeContext::gTol)
		.def("isScaledOrtho", &AcGeMatrix2d::isScaledOrtho,
			py::arg("tol") = AcGeContext::gTol)

		// 字符串表示
		.def("__repr__", [](const AcGeMatrix2d& m) {
		return std::format(
			"AcGeMatrix2d(\n"
			" [{:.6f}, {:.6f}, {:.6f}],\n"
			" [{:.6f}, {:.6f}, {:.6f}],\n"
			" [{:.6f}, {:.6f}, {:.6f}])",
			m.entry[0][0], m.entry[0][1], m.entry[0][2],
			m.entry[1][0], m.entry[1][1], m.entry[1][2],
			m.entry[2][0], m.entry[2][1], m.entry[2][2]);
			});
}
