#include "stdafx.h"
#include "PyGeVector3d.h"

#include <format>
#include <string>

void bind_AcGeVector3d(pybind11::module& m) {
	py::class_<AcGeVector3d>(m, "AcGeVector3d")
		// 构造函数
		.def(py::init<>())
		.def(py::init<const AcGeVector3d&>(), py::arg("vec"))
		.def(py::init<double, double, double>(), py::arg("x"), py::arg("y"), py::arg("z"))
		.def(py::init<const AcGePlanarEnt&, const AcGeVector2d&>(),
			py::arg("plane"), py::arg("vec2d"))

		// 静态常量
		.def_readonly_static("kIdentity", &AcGeVector3d::kIdentity)
		.def_readonly_static("kXAxis", &AcGeVector3d::kXAxis)
		.def_readonly_static("kYAxis", &AcGeVector3d::kYAxis)
		.def_readonly_static("kZAxis", &AcGeVector3d::kZAxis)

		// 矩阵变换
		.def("setToProduct",
			py::overload_cast<const AcGeMatrix3d&, const AcGeVector3d&>(&AcGeVector3d::setToProduct),
			py::arg("matrix"), py::arg("vector"),
			"Sets this vector to matrix * vector product")

		// 标量乘法（单独绑定另一个重载）
		.def("setToProduct",
			py::overload_cast<const AcGeVector3d&, double>(&AcGeVector3d::setToProduct),
			py::arg("vector"), py::arg("scalar"),
			"Sets this vector to vector * scalar product")

		.def("transformBy", &AcGeVector3d::transformBy,
			py::arg("matrix"))
		.def("rotateBy", &AcGeVector3d::rotateBy,
			py::arg("angle"), py::arg("axis"))
		.def("mirror", &AcGeVector3d::mirror,
			py::arg("normalToPlane"))

		// 标量运算（原生运算符绑定）
		.def(py::self * double())
		.def(double() * py::self)
		.def(py::self *= double())
		.def(py::self / double())
		.def(py::self /= double())

		// 向量代数
		.def(py::self + py::self)
		.def(py::self - py::self)
		.def(-py::self)
		.def(py::self += py::self)
		.def(py::self -= py::self)
		.def("setToSum", &AcGeVector3d::setToSum,
			py::arg("vec1"), py::arg("vec2"))

		// 向量运算
		.def("dotProduct", &AcGeVector3d::dotProduct,
			py::arg("vec"))
		.def("crossProduct", &AcGeVector3d::crossProduct,
			py::arg("vec"))
		.def("angleTo", py::overload_cast<const AcGeVector3d&>(&AcGeVector3d::angleTo, py::const_),
			py::arg("vec"))
		.def("angleTo", py::overload_cast<const AcGeVector3d&, const AcGeVector3d&>(&AcGeVector3d::angleTo, py::const_),
			py::arg("vec"), py::arg("refVec"))

		// 投影运算
		.def("project", py::overload_cast<const AcGeVector3d&, const AcGeVector3d&>(&AcGeVector3d::project, py::const_),
			py::arg("planeNormal"), py::arg("direction"))
		.def("orthoProject", py::overload_cast<const AcGeVector3d&>(&AcGeVector3d::orthoProject, py::const_),
			py::arg("planeNormal"))

		// 向量属性
		.def("length", &AcGeVector3d::length)
		.def("lengthSqrd", &AcGeVector3d::lengthSqrd)
		.def("normalize", py::overload_cast<const AcGeTol&>(&AcGeVector3d::normalize),
			py::arg("tol") = AcGeContext::gTol)
		.def("perpVector", &AcGeVector3d::perpVector)
		.def("negate", &AcGeVector3d::negate, "Negate this vector in place")
		.def("normal", &AcGeVector3d::normal,
			py::arg("tol") = AcGeContext::gTol, "Unit-length normal vector")

		// 长度 / 方向测试
		.def("isUnitLength", &AcGeVector3d::isUnitLength,
			py::arg("tol") = AcGeContext::gTol)
		.def("isZeroLength", &AcGeVector3d::isZeroLength,
			py::arg("tol") = AcGeContext::gTol)
		.def("isParallelTo",
			py::overload_cast<const AcGeVector3d&, const AcGeTol&>(&AcGeVector3d::isParallelTo, py::const_),
			py::arg("vec"), py::arg("tol") = AcGeContext::gTol)
		.def("isCodirectionalTo",
			py::overload_cast<const AcGeVector3d&, const AcGeTol&>(&AcGeVector3d::isCodirectionalTo, py::const_),
			py::arg("vec"), py::arg("tol") = AcGeContext::gTol)
		.def("isPerpendicularTo",
			py::overload_cast<const AcGeVector3d&, const AcGeTol&>(&AcGeVector3d::isPerpendicularTo, py::const_),
			py::arg("vec"), py::arg("tol") = AcGeContext::gTol)
		.def("rotateTo", &AcGeVector3d::rotateTo,
			py::arg("vec"), py::arg("axis") = AcGeVector3d::kIdentity,
			"Rotation matrix that maps this vector onto vec")

		// 比较运算
		.def("__eq__", &AcGeVector3d::operator==, py::arg("vec"))
		.def("__ne__", &AcGeVector3d::operator!=, py::arg("vec"))
		.def("isEqualTo", &AcGeVector3d::isEqualTo,
			py::arg("vec"), py::arg("tol") = AcGeContext::gTol)

		// 数据访问
		.def("__getitem__", [](const AcGeVector3d& v, int i) { return v[i]; })
		.def("__setitem__", [](AcGeVector3d& v, int i, double val) { v[i] = val; })
		.def("set", py::overload_cast<double, double, double>(&AcGeVector3d::set),
			py::arg("x"), py::arg("y"), py::arg("z"))
		.def("set", py::overload_cast<const AcGePlanarEnt&, const AcGeVector2d&>(&AcGeVector3d::set),
			py::arg("plane"), py::arg("vec2d"),
			"Set this vector from a 2D vector expressed on the given planar surface")
		// [ADD] SDK largestElement: index of the component with largest absolute value
		.def("largestElement", &AcGeVector3d::largestElement,
			"Return the index (0=x,1=y,2=z) of the component with the largest absolute value")

		// 成员变量
		.def_readwrite("x", &AcGeVector3d::x)
		.def_readwrite("y", &AcGeVector3d::y)
		.def_readwrite("z", &AcGeVector3d::z)
			.def("angleOnPlane", [](const AcGeVector3d& v, const AcGePlanarEnt& pln) {
				return v.angleOnPlane(pln);
				}, py::arg("plane"))
			.def("convert2d", [](const AcGeVector3d& v, const AcGePlanarEnt& pln) {
				return v.convert2d(pln);
				}, py::arg("plane"))

		// 字符串表示
		.def("__repr__", [](const AcGeVector3d& v) {
		return std::format("AcGeVector3d({:.3f}, {:.3f}, {:.3f})", v.x, v.y, v.z);
			});
}
