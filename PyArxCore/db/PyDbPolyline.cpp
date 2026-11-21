#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbPolyline.h"
#include "PyDb2dPolyline.h"
#include "dbents.h"
#include "dbsymtb.h"
#include "DbTrans.h"
#include <set>
#include <cmath>

PyWrapper_Cpp(DbPolyline)
AcString PyDbPolyline::toString() const { return AcString(std::format("AcDbPolyline[wrapper]").c_str(), AcString::Utf8); }
unsigned int PyDbPolyline::numVerts() const { if (!impObj()) return 0; return impObj() ? impObj()->numVerts() : 0; }

// 顶点索引边界校验（根因修复）：SDK 对越界索引不报错或返回默认值，
// 旧封装直接透传，空表读点/读凸度得到伪造的原点与 0。
static bool plReadIndexOk(const AcDbPolyline* p, unsigned int i) {
	if (!p) { PyWarnIfError(Acad::eNullObjectPointer, L"PyDbPolyline(null)"); return false; }
	if (i >= p->numVerts()) {
		PyWarnIfError(Acad::eInvalidInput, L"PyDbPolyline vertex index out of range");
		return false;
	}
	return true;
}

AcGePoint2d PyDbPolyline::getPointAt(unsigned int i) const {
	if (!impObj()) return AcGePoint2d();
	if (!plReadIndexOk(impObj(), i)) return AcGePoint2d();
	AcGePoint2d pt; if(!PyWarnIfError(impObj()->getPointAt(i, pt), L"AcDbPolyline::getPointAt")) return AcGePoint2d(); return pt;
}

bool  PyDbPolyline::setPointAt(unsigned int i, const AcGePoint2d& pt) {
	if (!impObj()) return false;
	if (!plReadIndexOk(impObj(), i)) return false;
	return PyWarnIfError(impObj()->setPointAt(i, pt), L"AcDbPolyline::setPointAt");
}

void PyDbPolyline::setClosed(bool c) { if (!impObj()) return; impObj()->setClosed(c ? Adesk::kTrue : Adesk::kFalse); }

double PyDbPolyline::getBulgeAt(unsigned int i) const {
	if (!impObj()) return 0.0;
	if (!plReadIndexOk(impObj(), i)) return 0.0;
	double v = 0.0; if(!PyWarnIfError(impObj()->getBulgeAt(i, v), L"AcDbPolyline::getBulgeAt")) return 0.0; return v;
}
bool  PyDbPolyline::setBulgeAt(unsigned int i, double b) {
	if (!impObj()) return false;
	if (!plReadIndexOk(impObj(), i)) return false;
	return PyWarnIfError(impObj()->setBulgeAt(i, b), L"AcDbPolyline::setBulgeAt");
}
py::tuple PyDbPolyline::getWidthsAt(unsigned int i) const {
	if (!impObj()) return py::tuple();
	if (!plReadIndexOk(impObj(), i)) return py::make_tuple(0.0, 0.0);
	double s = 0.0, e = 0.0; if(!PyWarnIfError(impObj()->getWidthsAt(i, s, e), L"AcDbPolyline::getWidthsAt")) return py::tuple(); return py::make_tuple(s, e);
}
bool  PyDbPolyline::setWidthsAt(unsigned int i, double sw, double ew) {
	if (!impObj()) return false;
	if (!plReadIndexOk(impObj(), i)) return false;
	return PyWarnIfError(impObj()->setWidthsAt(i, sw, ew), L"AcDbPolyline::setWidthsAt");
}
bool  PyDbPolyline::addVertexAt(unsigned int i, const AcGePoint2d& pt, double bulge, double sw, double ew) {
	auto* p = impObj();
	if (!p) { PyWarnIfError(Acad::eNullObjectPointer, L"PyDbPolyline(null)"); return false; }
	// addVertexAt 允许 0..numVerts（尾部插入），index > numVerts 为非法位置。
	if (i > p->numVerts()) {
		PyWarnIfError(Acad::eInvalidInput, L"PyDbPolyline.addVertexAt index > numVerts");
		return false;
	}
	return PyWarnIfError(p->addVertexAt(i, pt, bulge, sw, ew), L"AcDbPolyline::addVertexAt");
}
bool  PyDbPolyline::removeVertexAt(unsigned int i) {
	if (!impObj()) return false;
	if (!plReadIndexOk(impObj(), i)) return false;
	return PyWarnIfError(impObj()->removeVertexAt(i), L"AcDbPolyline::removeVertexAt");
}

double PyDbPolyline::elevation() const { if (!impObj()) return 0.0; return impObj()->elevation(); }
void PyDbPolyline::setElevation(double v) { if (!impObj()) return; impObj()->setElevation(v); }
double PyDbPolyline::thickness() const { if (!impObj()) return 0.0; return impObj()->thickness(); }
bool  PyDbPolyline::setThickness(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setThickness(v), L"AcDbPolyline::setThickness"); }
double PyDbPolyline::constantWidth() const { if (!impObj()) return 0.0; double v = 0.0; if(!PyWarnIfError(impObj()->getConstantWidth(v), L"AcDbPolyline::getConstantWidth")) return 0.0; return v; }
bool  PyDbPolyline::setConstantWidth(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setConstantWidth(v), L"AcDbPolyline::setConstantWidth"); }
AcGeVector3d PyDbPolyline::normal() const { if (!impObj()) return AcGeVector3d(); return impObj()->normal(); }
bool  PyDbPolyline::setNormal(const AcGeVector3d& n) { if (!impObj()) return false; return PyWarnIfError(impObj()->setNormal(n), L"AcDbPolyline::setNormal"); }
void PyDbPolyline::setPlinegen(bool on) { if (!impObj()) return; impObj()->setPlinegen(on ? Adesk::kTrue : Adesk::kFalse); }
bool PyDbPolyline::hasBulges() const { if (!impObj()) return false; return impObj()->hasBulges() != Adesk::kFalse; }
bool PyDbPolyline::hasWidth() const { if (!impObj()) return false; return impObj()->hasWidth() != Adesk::kFalse; }
bool PyDbPolyline::hasPlinegen() const { if (!impObj()) return false; return impObj()->hasPlinegen() != Adesk::kFalse; }
bool PyDbPolyline::isOnlyLines() const { if (!impObj()) return false; return impObj()->isOnlyLines() != Adesk::kFalse; }
bool PyDbPolyline::hasVertexIdentifiers() const { if (!impObj()) return false; return impObj()->hasVertexIdentifiers() != Adesk::kFalse; }
int PyDbPolyline::segType(unsigned int index) const {
	if (!impObj()) return 0;
	if (!plReadIndexOk(impObj(), index)) return -1;
	return static_cast<int>(impObj()->segType(index));
}
AcGeLineSeg2d PyDbPolyline::getLineSegAt(unsigned int index) const {
	if (!impObj()) return AcGeLineSeg2d();
	if (!plReadIndexOk(impObj(), index)) return AcGeLineSeg2d();
	AcGeLineSeg2d ln; if(!PyWarnIfError(impObj()->getLineSegAt(index, ln), L"AcDbPolyline::getLineSegAt")) return AcGeLineSeg2d(); return ln;
}
AcGeCircArc2d PyDbPolyline::getArcSegAt(unsigned int index) const {
	if (!impObj()) return AcGeCircArc2d();
	if (!plReadIndexOk(impObj(), index)) return AcGeCircArc2d();
	AcGeCircArc2d arc; if(!PyWarnIfError(impObj()->getArcSegAt(index, arc), L"AcDbPolyline::getArcSegAt")) return AcGeCircArc2d(); return arc;
}
AcGeMatrix3d PyDbPolyline::getEcs() const { if (!impObj()) return AcGeMatrix3d(); AcGeMatrix3d m; impObj()->getEcs(m); return m; }

std::tuple<bool, double> PyDbPolyline::onSegAt(unsigned int index, const AcGePoint2d& pt2d) const {
	if (!impObj()) return {};
	double param = 0.0;
	Adesk::Boolean on = impObj()->onSegAt(index, pt2d, param);
	return { on == Adesk::kTrue, param };
}

void PyDbPolyline::reset(bool reuse, unsigned int numVerts) {
	if (!impObj()) return;
	impObj()->reset(reuse ? Adesk::kTrue : Adesk::kFalse, numVerts);
}

bool PyDbPolyline::makeClosedIfStartAndEndVertexCoincide(double distTol) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->makeClosedIfStartAndEndVertexCoincide(distTol), L"AcDbPolyline::makeClosedIfStartAndEndVertexCoincide");
}

// 由 AcDb2dPolyline(或兼容实体) 填充本多段线。调用前本对象应尚未驻留数据库；
// transferId=true 成功后源实体被删除并置空，本对象接管其 ObjectId/句柄，源包装不可再用。
bool PyDbPolyline::convertFrom(const PyDbEntity& src, bool transferId) {
	if (!impObj()) return false;
	AcDbEntity* pSrc = src.impObj();
	if (!pSrc) return PyWarnIfError(Acad::eNullObjectPointer, L"AcDbPolyline::convertFrom");
	return PyWarnIfError(impObj()->convertFrom(pSrc, transferId ? Adesk::kTrue : Adesk::kFalse), L"AcDbPolyline::convertFrom");
}

// 生成本多段线对应的 AcDb2dPolyline 并交由返回的包装接管。
// transferId=true 成功返回 eObjectToBeDeleted：新对象接管 ObjectId，本对象变为非驻留、
// 由本包装析构时 delete 回收；transferId=false 成功返回 eOk，新对象未驻留、由包装 delete。
PyDb2dPolyline PyDbPolyline::convertTo(bool transferId) const {
	if (!impObj()) return {};
	AcDb2dPolyline* pResult = nullptr;
	Acad::ErrorStatus es = impObj()->convertTo(pResult, transferId ? Adesk::kTrue : Adesk::kFalse);
	if (es != Acad::eOk && es != Acad::eObjectToBeDeleted) {
		PyWarnIfError(es, L"AcDbPolyline::convertTo");
		return PyDb2dPolyline(nullptr);
	}
	return PyDb2dPolyline(pResult);
}

void bind_PyDbPolyline(pym& m) {
	py::class_<PyDbPolyline, PyDbCurve>(m, "PyDbPolyline")
		.def(py::init<>())
		.def(py::init<AcDbPolyline*>(), py::arg("object"))
		.def("numVerts", &PyDbPolyline::numVerts)
		.def("getPointAt", &PyDbPolyline::getPointAt, py::arg("index"))
		.def("setPointAt", &PyDbPolyline::setPointAt, py::arg("index"), py::arg("pt"))
		.def("setClosed", &PyDbPolyline::setClosed, py::arg("closed"))
		.def("getBulgeAt", &PyDbPolyline::getBulgeAt, py::arg("index"))
		.def("setBulgeAt", &PyDbPolyline::setBulgeAt, py::arg("index"), py::arg("bulge"))
		.def("getWidthsAt", &PyDbPolyline::getWidthsAt, py::arg("index"))
		.def("setWidthsAt", &PyDbPolyline::setWidthsAt, py::arg("index"), py::arg("startWidth"), py::arg("endWidth"))
		.def("addVertexAt", &PyDbPolyline::addVertexAt, py::arg("index"), py::arg("pt"), py::arg("bulge") = 0.0, py::arg("startWidth") = -1.0, py::arg("endWidth") = -1.0)
		.def("removeVertexAt", &PyDbPolyline::removeVertexAt, py::arg("index"))
		.def("elevation", &PyDbPolyline::elevation)
		.def("setElevation", &PyDbPolyline::setElevation, py::arg("v"))
		.def("thickness", &PyDbPolyline::thickness)
		.def("setThickness", &PyDbPolyline::setThickness, py::arg("v"))
		.def("constantWidth", &PyDbPolyline::constantWidth)
		.def("setConstantWidth", &PyDbPolyline::setConstantWidth, py::arg("v"))
		.def("normal", &PyDbPolyline::normal)
		.def("setNormal", &PyDbPolyline::setNormal, py::arg("n"))
		.def("setPlinegen", &PyDbPolyline::setPlinegen, py::arg("on"))
		.def("hasBulges", &PyDbPolyline::hasBulges)
		.def("hasWidth", &PyDbPolyline::hasWidth)
		.def("hasPlinegen", &PyDbPolyline::hasPlinegen)
		.def("isOnlyLines", &PyDbPolyline::isOnlyLines)
		.def("hasVertexIdentifiers", &PyDbPolyline::hasVertexIdentifiers)
		.def("segType", &PyDbPolyline::segType, py::arg("index"))
		.def("getLineSegAt", &PyDbPolyline::getLineSegAt, py::arg("index"))
		.def("getArcSegAt", &PyDbPolyline::getArcSegAt, py::arg("index"))
		.def("getEcs", &PyDbPolyline::getEcs)
		.def("onSegAt", &PyDbPolyline::onSegAt, py::arg("index"), py::arg("pt2d"))
		.def("reset", &PyDbPolyline::reset, py::arg("reuse"), py::arg("numVerts"))
		.def("makeClosedIfStartAndEndVertexCoincide", &PyDbPolyline::makeClosedIfStartAndEndVertexCoincide, py::arg("distTol"))
		.def("convertFrom", &PyDbPolyline::convertFrom, py::arg("src"), py::arg("transferId") = true)
		.def("convertTo", &PyDbPolyline::convertTo, py::arg("transferId") = true)
		.def_static("join", &PyDbPolyline::join, "Merge connected LINE/ARC/LWPOLYLINE in current space into new LWPOLYLINEs")
		PyWrapper_BIND(DbPolyline)
		.def("__repr__", &PyDbPolyline::toString)
		;
}

// ── JOIN 算法实现 ──────────────────────────────────────────────
namespace {
struct JoinSeg {
	AcGePoint3d s, e;
	double bulge;
	AcDbObjectId id;
};
static bool _ptEq(const AcGePoint3d& a, const AcGePoint3d& b, double tol = 1e-4) {
	return fabs(a.x - b.x) < tol && fabs(a.y - b.y) < tol;
}
}

int PyDbPolyline::join() {
	const double PI = 3.14159265358979323846;
	DbTrans T;
	if (!T.pDb) return 0;

	// 收集段
	std::vector<JoinSeg> segs;
	{
		AcDbBlockTableRecord* pBtr = T.getObject<AcDbBlockTableRecord>(T.pDb->currentSpaceId(), AcDb::kForRead);
		if (!pBtr) return 0;
		AcDbBlockTableRecordIterator* pIt = nullptr;
		pBtr->newIterator(pIt);
		for (pIt->start(true, true); !pIt->done(); pIt->step(true, true)) {
			AcDbObjectId eid;
			if (pIt->getEntityId(eid) != Acad::eOk) continue;
			AcDbEntity* pEnt = nullptr;
			if (T.pTrans->getObject(pEnt, eid, AcDb::kForRead) != Acad::eOk || !pEnt) continue;

			if (pEnt->isA() == AcDbLine::desc()) {
				AcDbLine* pL = static_cast<AcDbLine*>(pEnt);
				segs.push_back({pL->startPoint(), pL->endPoint(), 0.0, eid});
			} else if (pEnt->isA() == AcDbArc::desc()) {
				AcDbArc* pA = static_cast<AcDbArc*>(pEnt);
				AcGePoint3d c = pA->center(); double r = pA->radius();
				double a0 = pA->startAngle(), a1 = pA->endAngle();
				AcGePoint3d s(c.x + r*cos(a0), c.y + r*sin(a0), 0);
				AcGePoint3d e(c.x + r*cos(a1), c.y + r*sin(a1), 0);
				double dt = a1 - a0; while (dt <= 0) dt += 2*PI;
				segs.push_back({s, e, tan(dt/4.0), eid});
			} else if (pEnt->isA() == AcDbPolyline::desc()) {
				AcDbPolyline* pP = static_cast<AcDbPolyline*>(pEnt);
				unsigned n = pP->numVerts();
				for (unsigned i = 0; i + 1 < n; ++i) {
					AcGePoint2d p0, p1;
					pP->getPointAt(i, p0);
					pP->getPointAt(i+1, p1);
					double b = 0.0;
					pP->getBulgeAt(i, b);
					segs.push_back({AcGePoint3d(p0.x, p0.y, 0), AcGePoint3d(p1.x, p1.y, 0), b, eid});
				}
			}
		}
		delete pIt;
		pBtr->close();
	}

	// 贪心连接成链
	std::vector<bool> used(segs.size(), false);
	std::vector<std::vector<size_t>> chains;
	for (size_t i = 0; i < segs.size(); ++i) {
		if (used[i]) continue;
		std::vector<size_t> chain = {i};
		used[i] = true;
		bool changed = true;
		while (changed) {
			changed = false;
			AcGePoint3d head = segs[chain.front()].s;
			AcGePoint3d tail = segs[chain.back()].e;
			for (size_t k = 0; k < segs.size(); ++k) {
				if (used[k]) continue;
				AcGePoint3d s = segs[k].s, e = segs[k].e;
				if (_ptEq(s, tail)) {
					chain.push_back(k); used[k] = true; changed = true;
				} else if (_ptEq(e, tail)) {
					segs[k].s = e; segs[k].e = s; segs[k].bulge = -segs[k].bulge;
					chain.push_back(k); used[k] = true; changed = true;
				} else if (_ptEq(e, head)) {
					chain.insert(chain.begin(), k); used[k] = true; changed = true;
				} else if (_ptEq(s, head)) {
					segs[k].s = e; segs[k].e = s; segs[k].bulge = -segs[k].bulge;
					chain.insert(chain.begin(), k); used[k] = true; changed = true;
				}
			}
		}
		chains.push_back(chain);
	}

	// 建 polyline + erase
	int merged = 0;
	AcDbBlockTableRecord* pBtr = T.getObject<AcDbBlockTableRecord>(T.pDb->currentSpaceId(), AcDb::kForWrite);
	if (!pBtr) return 0;
	for (auto& chain : chains) {
		if (chain.size() < 2) continue;
		AcDbPolyline* pNew = new AcDbPolyline();
		for (size_t idx = 0; idx < chain.size(); ++idx) {
			auto& sg = segs[chain[idx]];
			pNew->addVertexAt((int)idx, AcGePoint2d(sg.s.x, sg.s.y), sg.bulge);
		}
		auto& last = segs[chain.back()];
		pNew->addVertexAt((int)chain.size(), AcGePoint2d(last.e.x, last.e.y), 0.0);
		T.addEntity(pBtr, pNew);
		std::set<AcDbObjectId> erased;
		for (auto ci : chain) {
			AcDbObjectId oid = segs[ci].id;
			if (erased.count(oid)) continue;
			erased.insert(oid);
			AcDbEntity* pOld = T.getObject<AcDbEntity>(oid, AcDb::kForWrite);
			if (pOld) T.erase(pOld);
		}
		++merged;
	}
	pBtr->close();
	return merged;
}

