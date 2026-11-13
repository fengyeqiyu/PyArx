#include "stdafx.h"
#include "PyGeMatrix3d.h"

#include <format>
#include <string>

void bind_AcGeMatrix3d(pybind11::module& m) {
	py::class_<AcGeMatrix3d>(m, "AcGeMatrix3d")
		// 构造函数
		.def(py::init<>())
		.def(py::init<const AcGeMatrix3d&>(), py::arg("src"))
		// 常量与基础操作
		.def_readonly_static("kIdentity", &AcGeMatrix3d::kIdentity)
		.def("setToIdentity", &AcGeMatrix3d::setToIdentity)

		// 矩阵运算
		.def(py::self * py::self)
		.def(py::self *= py::self)
		.def("preMultBy", &AcGeMatrix3d::preMultBy, py::arg("leftSide"))
		.def("postMultBy", &AcGeMatrix3d::postMultBy, py::arg("rightSide"))
		.def("setToProduct", &AcGeMatrix3d::setToProduct,
			py::arg("mat1"), py::arg("mat2"))

		// 矩阵特性
		.def("invert", &AcGeMatrix3d::invert)
		.def("inverse", py::overload_cast<>(&AcGeMatrix3d::inverse, py::const_))
		.def("inverse", py::overload_cast<const AcGeTol&>(&AcGeMatrix3d::inverse, py::const_),
			py::arg("tol"))
		// SDK 第三重载: Adesk::Boolean inverse(AcGeMatrix3d& inv, double tol) const
		// 出参版本: 返回 (成功标志, 逆矩阵)；奇异时逆矩阵回退 kIdentity 并在命令行提示，不抛异常。
		.def("inverseInto", [](const AcGeMatrix3d& self, double tol) {
			AcGeMatrix3d inv = AcGeMatrix3d::kIdentity;
			Adesk::Boolean ok = self.inverse(inv, tol);
			if (ok == Adesk::kFalse) {
				acutPrintf(L"[PyArx] AcGeMatrix3d::inverseInto 矩阵奇异，返回 kIdentity\n");
				inv = AcGeMatrix3d::kIdentity;
			}
			return py::make_tuple(ok != Adesk::kFalse, inv);
		}, py::arg("tol") = AcGeContext::gTol.equalPoint(),
			"Return (success, inverseMatrix); singular matrix returns (false, kIdentity)")
		.def("isSingular", &AcGeMatrix3d::isSingular,
			py::arg("tol") = AcGeContext::gTol)
		.def("transposeIt", &AcGeMatrix3d::transposeIt)
		.def("transpose", &AcGeMatrix3d::transpose)
		.def("det", &AcGeMatrix3d::det)
		.def("scale", &AcGeMatrix3d::scale)
		.def("norm", &AcGeMatrix3d::norm)
		.def("isUniScaledOrtho", &AcGeMatrix3d::isUniScaledOrtho,
			py::arg("tol") = AcGeContext::gTol)
		.def("isScaledOrtho", &AcGeMatrix3d::isScaledOrtho,
			py::arg("tol") = AcGeContext::gTol)

		// 变换操作
		.def("setTranslation", &AcGeMatrix3d::setTranslation, py::arg("vec"))
		.def("translation",
			py::overload_cast<>(&AcGeMatrix3d::translation, py::const_),
			"Returns the translation vector component of this matrix")
		.def("setToTranslation", &AcGeMatrix3d::setToTranslation, py::arg("vec"))
		.def("setToRotation", &AcGeMatrix3d::setToRotation,
			py::arg("angle"), py::arg("axis"),
			py::arg("center") = AcGePoint3d::kOrigin)
		.def("setToScaling", &AcGeMatrix3d::setToScaling,
			py::arg("scale"), py::arg("center") = AcGePoint3d::kOrigin)
		.def("setToMirroring", py::overload_cast<const AcGePlane&>(&AcGeMatrix3d::setToMirroring),
			py::arg("plane"))
		.def("setToMirroring", py::overload_cast<const AcGePoint3d&>(&AcGeMatrix3d::setToMirroring),
			py::arg("point"))
		.def("setToMirroring", py::overload_cast<const AcGeLine3d&>(&AcGeMatrix3d::setToMirroring),
			py::arg("line"))
		.def("setToProjection", &AcGeMatrix3d::setToProjection,
			py::arg("plane"), py::arg("direction"))
		// [ADD] SDK instance setters: world<->plane basis conversion
		.def("setToWorldToPlane", py::overload_cast<const AcGeVector3d&>(&AcGeMatrix3d::setToWorldToPlane),
			py::arg("normal"),
			"Set this matrix to a world-to-plane transform built from a plane normal")
		.def("setToWorldToPlane", py::overload_cast<const AcGePlane&>(&AcGeMatrix3d::setToWorldToPlane),
			py::arg("plane"),
			"Set this matrix to a world-to-plane transform built from a plane")
		.def("setToPlaneToWorld", py::overload_cast<const AcGeVector3d&>(&AcGeMatrix3d::setToPlaneToWorld),
			py::arg("normal"),
			"Set this matrix to a plane-to-world transform built from a plane normal")
		.def("setToPlaneToWorld", py::overload_cast<const AcGePlane&>(&AcGeMatrix3d::setToPlaneToWorld),
			py::arg("plane"),
			"Set this matrix to a plane-to-world transform built from a plane")

		// 坐标系转换
		.def("setCoordSystem", &AcGeMatrix3d::setCoordSystem,
			py::arg("origin"), py::arg("xAxis"), py::arg("yAxis"), py::arg("zAxis"))
		.def("getCoordSystem", [](const AcGeMatrix3d& self) {
		AcGePoint3d origin;
		AcGeVector3d x, y, z;
		self.getCoordSystem(origin, x, y, z);
		return py::make_tuple(origin, x, y, z);
			})
		.def("setToAlignCoordSys", &AcGeMatrix3d::setToAlignCoordSys,
			py::arg("fromOrigin"), py::arg("fromX"), py::arg("fromY"), py::arg("fromZ"),
			py::arg("toOrigin"), py::arg("toX"), py::arg("toY"), py::arg("toZ"))
		.def("convertToLocal", &AcGeMatrix3d::convertToLocal,
			py::arg("normal"), py::arg("elevation"))

		// 静态构造方法
		.def_static("move", py::overload_cast<const AcGeVector3d&>(&AcGeMatrix3d::translation),
			py::arg("vector"),
			"Creates a translation matrix from 3D vector")
		.def_static("rotation", &AcGeMatrix3d::rotation,
			py::arg("angle"), py::arg("axis"),
			py::arg("center") = AcGePoint3d::kOrigin)
		.def_static("scaling", &AcGeMatrix3d::scaling,
			py::arg("scale"), py::arg("center") = AcGePoint3d::kOrigin)
		.def_static("mirroring", py::overload_cast<const AcGePlane&>(&AcGeMatrix3d::mirroring),
			py::arg("plane"))
		.def_static("mirroring", py::overload_cast<const AcGePoint3d&>(&AcGeMatrix3d::mirroring),
			py::arg("point"))
		.def_static("mirroring", py::overload_cast<const AcGeLine3d&>(&AcGeMatrix3d::mirroring),
			py::arg("line"))
		.def_static("projection", &AcGeMatrix3d::projection,
			py::arg("plane"), py::arg("direction"))
		.def_static("alignCoordSys", &AcGeMatrix3d::alignCoordSys,
			py::arg("fromOrigin"), py::arg("fromX"), py::arg("fromY"), py::arg("fromZ"),
			py::arg("toOrigin"), py::arg("toX"), py::arg("toY"), py::arg("toZ"))
		.def_static("worldToPlane", py::overload_cast<const AcGeVector3d&>(&AcGeMatrix3d::worldToPlane),
			py::arg("normal"))
		.def_static("worldToPlane", py::overload_cast<const AcGePlane&>(&AcGeMatrix3d::worldToPlane),
			py::arg("plane"))
		.def_static("planeToWorld", py::overload_cast<const AcGeVector3d&>(&AcGeMatrix3d::planeToWorld),
			py::arg("normal"))
		.def_static("planeToWorld", py::overload_cast<const AcGePlane&>(&AcGeMatrix3d::planeToWorld),
			py::arg("plane"))

		// 矩阵访问
		.def("__call__", [](const AcGeMatrix3d& m, unsigned int r, unsigned int c) {
		if (r >= 4 || c >= 4) throw py::index_error();
		return m(r, c);
			}, py::arg("row"), py::arg("col"))
		// [ADD] tuple-indexed read access, mirroring __setitem__
		.def("__getitem__", [](const AcGeMatrix3d& m, py::tuple idx) {
		unsigned int r = idx[0].cast<unsigned int>();
		unsigned int c = idx[1].cast<unsigned int>();
		if (r >= 4 || c >= 4) throw py::index_error();
		return m(r, c);
			}, py::arg("row_col"))
		.def("__setitem__", [](AcGeMatrix3d& m, py::tuple idx, double val) {
		unsigned int r = idx[0].cast<unsigned int>();
		unsigned int c = idx[1].cast<unsigned int>();
		if (r >= 4 || c >= 4) throw py::index_error();
		m(r, c) = val;
			})
		.def("toNumpy", [](const AcGeMatrix3d& m) {
		// numpy 可用时返回 4x4 ndarray；加载失败(debug/release ABI 不匹配)时回退嵌套 list，不崩不抛。
		try {
			py::array_t<double> arr({ 4, 4 });
			auto r = arr.mutable_unchecked<2>();
			for (int i = 0; i < 4; ++i)
				for (int j = 0; j < 4; ++j)
					r(i, j) = m.entry[i][j];
			return py::object(arr);
		} catch (...) {
			py::list rows;
			for (int i = 0; i < 4; ++i) {
				py::list row;
				for (int j = 0; j < 4; ++j) row.append(m.entry[i][j]);
				rows.append(row);
			}
			return py::object(rows);
		}
			}, "convert to 4x4 numpy array (falls back to nested list if numpy unavailable)")

		// 比较运算
		.def(py::self == py::self)
		.def(py::self != py::self)
		.def("isEqualTo", &AcGeMatrix3d::isEqualTo,
			py::arg("mat"), py::arg("tol") = AcGeContext::gTol)

		// 字符串表示
		.def("__repr__", [](const AcGeMatrix3d& m) {
		return std::format(
			"AcGeMatrix3d(\n"
			" [{:.6f}, {:.6f}, {:.6f}, {:.6f}],\n"
			" [{:.6f}, {:.6f}, {:.6f}, {:.6f}],\n"
			" [{:.6f}, {:.6f}, {:.6f}, {:.6f}],\n"
			" [{:.6f}, {:.6f}, {:.6f}, {:.6f}])",
			m.entry[0][0], m.entry[0][1], m.entry[0][2], m.entry[0][3],
			m.entry[1][0], m.entry[1][1], m.entry[1][2], m.entry[1][3],
			m.entry[2][0], m.entry[2][1], m.entry[2][2], m.entry[2][3],
			m.entry[3][0], m.entry[3][1], m.entry[3][2], m.entry[3][3]);
			});
}
