#include "stdafx.h"
#include "PyGeVector2d.h"

#include <format>
#include <string>

void bind_AcGeVector2d(pybind11::module& m) {
	py::class_<AcGeVector2d>(m, "AcGeVector2d")
		// 构造函数
		.def(py::init<>())
		.def(py::init<const AcGeVector2d&>(), py::arg("src"))
		.def(py::init<double, double>(), py::arg("x"), py::arg("y"))

		// 静态常量
		.def_readonly_static("kIdentity", &AcGeVector2d::kIdentity)
		.def_readonly_static("kXAxis", &AcGeVector2d::kXAxis)
		.def_readonly_static("kYAxis", &AcGeVector2d::kYAxis)

		// 矩阵变换（解决setToProduct重载问题）
		.def("setToMatrixProduct",
			py::overload_cast<const AcGeMatrix2d&, const AcGeVector2d&>(&AcGeVector2d::setToProduct),
			py::arg("matrix"), py::arg("vector"))
		.def("setToScalarProduct",
			py::overload_cast<const AcGeVector2d&, double>(&AcGeVector2d::setToProduct),
			py::arg("vector"), py::arg("scalar"))
		.def("transformBy", &AcGeVector2d::transformBy, py::arg("matrix"))
		.def("rotateBy", &AcGeVector2d::rotateBy, py::arg("angle"))
		.def("mirror", &AcGeVector2d::mirror, py::arg("mirrorLine"))

		// 标量运算
		.def(py::self * double())
		.def(double() * py::self)
		.def(py::self *= double())
		.def(py::self / double())
		.def(py::self /= double())

		// 向量运算
		.def(py::self + py::self)
		.def(py::self - py::self)
		.def(-py::self)
		.def(py::self += py::self)
		.def(py::self -= py::self)
		.def("setToSum", &AcGeVector2d::setToSum,
			py::arg("vec1"), py::arg("vec2"))
		.def("negate", &AcGeVector2d::negate)

		// 向量操作
		.def("perpVector", &AcGeVector2d::perpVector)
		.def("angle", &AcGeVector2d::angle)
		.def("angleTo", &AcGeVector2d::angleTo, py::arg("other"))

		// 向量属性
		.def("normal", &AcGeVector2d::normal, py::arg("tol") = AcGeContext::gTol)
		.def("normalize", py::overload_cast<const AcGeTol&>(&AcGeVector2d::normalize),
			py::arg("tol") = AcGeContext::gTol)
		.def("length", &AcGeVector2d::length)
		.def("lengthSqrd", &AcGeVector2d::lengthSqrd)

		// 方向测试
		.def("isParallelTo",
			py::overload_cast<const AcGeVector2d&, const AcGeTol&>(&AcGeVector2d::isParallelTo, py::const_),
			py::arg("other"), py::arg("tol") = AcGeContext::gTol)
		.def("isPerpendicularTo",
			py::overload_cast<const AcGeVector2d&, const AcGeTol&>(&AcGeVector2d::isPerpendicularTo, py::const_),
			py::arg("other"), py::arg("tol") = AcGeContext::gTol)

		// 点积和叉积（2D 叉积为标量：z 分量）
		.def("dotProduct", &AcGeVector2d::dotProduct, py::arg("other"))
		.def("crossProduct", &AcGeVector2d::crossProduct, py::arg("other"))

		// 长度 / 方向测试补充
		.def("isUnitLength", &AcGeVector2d::isUnitLength,
			py::arg("tol") = AcGeContext::gTol)
		.def("isZeroLength", &AcGeVector2d::isZeroLength,
			py::arg("tol") = AcGeContext::gTol)
		.def("isCodirectionalTo",
			py::overload_cast<const AcGeVector2d&, const AcGeTol&>(&AcGeVector2d::isCodirectionalTo, py::const_),
			py::arg("other"), py::arg("tol") = AcGeContext::gTol)

		// 比较运算
		.def(py::self == py::self)
		.def(py::self != py::self)
		.def("isEqualTo", &AcGeVector2d::isEqualTo,
			py::arg("other"), py::arg("tol") = AcGeContext::gTol)

		// 数据访问
		.def("__getitem__", [](const AcGeVector2d& v, int i) { return v[i]; })
		.def("__setitem__", [](AcGeVector2d& v, int i, double val) { v[i] = val; })
		.def("set", &AcGeVector2d::set, py::arg("x"), py::arg("y"))
		.def_readwrite("x", &AcGeVector2d::x)
		.def_readwrite("y", &AcGeVector2d::y)

		// 字符串表示
		.def("__repr__", [](const AcGeVector2d& v) {
		return std::format("AcGeVector2d({:.3f}, {:.3f})", v.x, v.y);
			});
}
