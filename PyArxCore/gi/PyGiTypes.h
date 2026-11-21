#pragma once
// ===========================================================================
// PyGiTypes.h —— AcGi 绑定层的 Python → AcGe 值类型转换 helper。
//
// 目的：AcGi 的绘制接口（AcGiGeometry / AcGiSubEntityTraits / AcGiViewport）
// 参数全是 AcGe 值类型（Point3d / Vector3d / Matrix3d / Point2d）。Python 侧
// 既允许传 PyArxCore.Ge 的 AcGePoint3d 等对象，也允许传 (x, y[, z]) 元组/列表，
// 这样脚本不用为了画一条线去构造 4 个几何对象。
//
// 约定：只做“入参”转换；返回值一律是真实的 AcGe 类型（由 PyGe 子模块注册），
// Python 侧可以直接拿到 AcGePoint3d / AcGeMatrix3d 继续运算。
// ===========================================================================
#include "stdafx.h"

// 点：AcGePoint3d | AcGePoint2d | (x, y) | (x, y, z)
AcGePoint3d AsPoint3d(const py::handle& h);
// 向量：AcGeVector3d | (x, y) | (x, y, z)
AcGeVector3d AsVector3d(const py::handle& h);
// 二维点：AcGePoint2d | AcGePoint3d | (x, y)
AcGePoint2d AsPoint2d(const py::handle& h);
// 矩阵：AcGeMatrix3d | 16 个数的平铺序列 | 4x4 嵌套序列（行优先）
AcGeMatrix3d AsMatrix3d(const py::handle& h);

// 点数组：[(x,y,z) | AcGePoint3d ...]；点数不足报 ValueError
std::vector<AcGePoint3d> AsPoint3dList(const py::handle& h, size_t minCount = 1, const char* what = "points");
// 法向数组：长度与点数组一致时允许 1 个元素复用（单法向），否则逐点对应
std::vector<AcGeVector3d> AsVector3dList(const py::handle& h, size_t expected);
