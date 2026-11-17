#include "stdafx.h"
#include "PyGiTypes.h"

namespace {

bool IsSeq(const py::handle& h) {
	return py::isinstance<py::sequence>(h) && !py::isinstance<py::str>(h);
}

double NumAt(const py::sequence& s, size_t i) {
	return py::cast<double>(s[static_cast<py::ssize_t>(i)]);
}

} // namespace

AcGePoint3d AsPoint3d(const py::handle& h) {
	if (py::isinstance<AcGePoint3d>(h))
		return py::cast<AcGePoint3d>(h);
	if (py::isinstance<AcGePoint2d>(h)) {
		const AcGePoint2d p = py::cast<AcGePoint2d>(h);
		return AcGePoint3d(p.x, p.y, 0.0);
	}
	if (!IsSeq(h))
		throw py::type_error("需要 AcGePoint3d 或 (x, y[, z]) 序列");
	const py::sequence s = py::reinterpret_borrow<py::sequence>(h);
	const size_t n = static_cast<size_t>(py::len(s));
	if (n < 2 || n > 3)
		throw py::value_error("点必须是 (x, y) 或 (x, y, z)");
	return AcGePoint3d(NumAt(s, 0), NumAt(s, 1), n == 3 ? NumAt(s, 2) : 0.0);
}

AcGeVector3d AsVector3d(const py::handle& h) {
	if (py::isinstance<AcGeVector3d>(h))
		return py::cast<AcGeVector3d>(h);
	if (!IsSeq(h))
		throw py::type_error("需要 AcGeVector3d 或 (x, y[, z]) 序列");
	const py::sequence s = py::reinterpret_borrow<py::sequence>(h);
	const size_t n = static_cast<size_t>(py::len(s));
	if (n < 2 || n > 3)
		throw py::value_error("向量必须是 (x, y) 或 (x, y, z)");
	return AcGeVector3d(NumAt(s, 0), NumAt(s, 1), n == 3 ? NumAt(s, 2) : 0.0);
}

AcGePoint2d AsPoint2d(const py::handle& h) {
	if (py::isinstance<AcGePoint2d>(h))
		return py::cast<AcGePoint2d>(h);
	if (py::isinstance<AcGePoint3d>(h)) {
		const AcGePoint3d p = py::cast<AcGePoint3d>(h);
		return AcGePoint2d(p.x, p.y);
	}
	if (!IsSeq(h))
		throw py::type_error("需要 AcGePoint2d 或 (x, y) 序列");
	const py::sequence s = py::reinterpret_borrow<py::sequence>(h);
	const size_t n = static_cast<size_t>(py::len(s));
	if (n < 2)
		throw py::value_error("二维点必须是 (x, y)");
	return AcGePoint2d(NumAt(s, 0), NumAt(s, 1));
}

AcGeMatrix3d AsMatrix3d(const py::handle& h) {
	if (py::isinstance<AcGeMatrix3d>(h))
		return py::cast<AcGeMatrix3d>(h);
	if (!IsSeq(h))
		throw py::type_error("需要 AcGeMatrix3d 或 4x4 / 16 元素序列");
	const py::sequence s = py::reinterpret_borrow<py::sequence>(h);
	const size_t n = static_cast<size_t>(py::len(s));
	AcGeMatrix3d m = AcGeMatrix3d::kIdentity;
	if (n == 16) {
		// 行优先平铺：m[row][col]
		for (size_t r = 0; r < 4; ++r)
			for (size_t c = 0; c < 4; ++c)
				m(static_cast<unsigned>(r), static_cast<unsigned>(c)) = NumAt(s, r * 4 + c);
		return m;
	}
	if (n == 4) {
		for (size_t r = 0; r < 4; ++r) {
			const py::sequence row = py::cast<py::sequence>(s[static_cast<py::ssize_t>(r)]);
			if (static_cast<size_t>(py::len(row)) != 4)
				throw py::value_error("矩阵每行必须有 4 个元素");
			for (size_t c = 0; c < 4; ++c)
				m(static_cast<unsigned>(r), static_cast<unsigned>(c)) = NumAt(row, c);
		}
		return m;
	}
	throw py::value_error("矩阵必须是 4x4 嵌套序列或 16 元素平铺序列");
}

std::vector<AcGePoint3d> AsPoint3dList(const py::handle& h, size_t minCount, const char* what) {
	if (!IsSeq(h))
		throw py::type_error(std::string(what) + " 需要点序列");
	const py::sequence s = py::reinterpret_borrow<py::sequence>(h);
	const size_t n = static_cast<size_t>(py::len(s));
	if (n < minCount)
		throw py::value_error(std::string(what) + " 点数不足");
	std::vector<AcGePoint3d> pts;
	pts.reserve(n);
	for (size_t i = 0; i < n; ++i)
		pts.push_back(AsPoint3d(s[static_cast<py::ssize_t>(i)]));
	return pts;
}

std::vector<AcGeVector3d> AsVector3dList(const py::handle& h, size_t expected) {
	if (h.is_none())
		return {};
	if (!IsSeq(h))
		throw py::type_error("法向需要 AcGeVector3d 或向量序列");
	const py::sequence s = py::reinterpret_borrow<py::sequence>(h);
	const size_t n = static_cast<size_t>(py::len(s));
	std::vector<AcGeVector3d> vecs;
	vecs.reserve(n);
	for (size_t i = 0; i < n; ++i)
		vecs.push_back(AsVector3d(s[static_cast<py::ssize_t>(i)]));
	if (vecs.size() != 1 && vecs.size() != expected)
		throw py::value_error("法向数量必须为 1 或与点数一致");
	return vecs;
}
