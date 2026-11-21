#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "dbSweepOptions.h"
#include "PyDbEntity.h"
#include "PyDbSweepOptions.h"

PyDbSweepOptions::PyDbSweepOptions() : m_pObj(new AcDbSweepOptions()) {}
PyDbSweepOptions::PyDbSweepOptions(AcDbSweepOptions* p) : m_pObj(p) {}
PyDbSweepOptions::~PyDbSweepOptions() { delete m_pObj; }
AcDbSweepOptions* PyDbSweepOptions::impObj() { return m_pObj; }
const AcDbSweepOptions* PyDbSweepOptions::impObj() const { return m_pObj; }

void bind_PyDbSweepOptions(pym& m) {
	py::class_<PyDbSweepOptions>(m, "PyDbSweepOptions")
		.def(py::init<>(), "默认构造（内部 new AcDbSweepOptions）")
		// ---- AlignOption 枚举值（静态常量） ----
		.def_property_readonly_static("kNoAlignment", [](py::object){ return AcDbSweepOptions::kNoAlignment; })
		.def_property_readonly_static("kAlignSweepEntityToPath", [](py::object){ return AcDbSweepOptions::kAlignSweepEntityToPath; })
		.def_property_readonly_static("kTranslateSweepEntityToPath", [](py::object){ return AcDbSweepOptions::kTranslateSweepEntityToPath; })
		.def_property_readonly_static("kTranslatePathToSweepEntity", [](py::object){ return AcDbSweepOptions::kTranslatePathToSweepEntity; })
		// ---- MiterOption 枚举值 ----
		.def_property_readonly_static("kDefaultMiter", [](py::object){ return AcDbSweepOptions::kDefaultMiter; })
		.def_property_readonly_static("kOldMiter", [](py::object){ return AcDbSweepOptions::kOldMiter; })
		.def_property_readonly_static("kNewMiter", [](py::object){ return AcDbSweepOptions::kNewMiter; })
		.def_property_readonly_static("kCrimpMiter", [](py::object){ return AcDbSweepOptions::kCrimpMiter; })
		.def_property_readonly_static("kBendMiter", [](py::object){ return AcDbSweepOptions::kBendMiter; })

		// ---- draftAngle ----
		.def("draftAngle", [](PyDbSweepOptions& s){ return s.impObj()->draftAngle(); })
		.def("setDraftAngle", [](PyDbSweepOptions& s, double ang){ s.impObj()->setDraftAngle(ang); return true; }, py::arg("ang"))
		// ---- startDraftDist ----
		.def("startDraftDist", [](PyDbSweepOptions& s){ return s.impObj()->startDraftDist(); })
		.def("setStartDraftDist", [](PyDbSweepOptions& s, double v){ s.impObj()->setStartDraftDist(v); return true; }, py::arg("v"))
		// ---- endDraftDist ----
		.def("endDraftDist", [](PyDbSweepOptions& s){ return s.impObj()->endDraftDist(); })
		.def("setEndDraftDist", [](PyDbSweepOptions& s, double v){ s.impObj()->setEndDraftDist(v); return true; }, py::arg("v"))
		// ---- twistAngle ----
		.def("twistAngle", [](PyDbSweepOptions& s){ return s.impObj()->twistAngle(); })
		.def("setTwistAngle", [](PyDbSweepOptions& s, double ang){ s.impObj()->setTwistAngle(ang); return true; }, py::arg("ang"))
		// ---- scaleFactor ----
		.def("scaleFactor", [](PyDbSweepOptions& s){ return s.impObj()->scaleFactor(); })
		.def("setScaleFactor", [](PyDbSweepOptions& s, double v){ s.impObj()->setScaleFactor(v); return true; }, py::arg("v"))
		// ---- alignAngle ----
		.def("alignAngle", [](PyDbSweepOptions& s){ return s.impObj()->alignAngle(); })
		.def("setAlignAngle", [](PyDbSweepOptions& s, double ang){ s.impObj()->setAlignAngle(ang); return true; }, py::arg("ang"))
		// ---- align（AlignOption） ----
		.def("align", [](PyDbSweepOptions& s){ return static_cast<int>(s.impObj()->align()); })
		.def("setAlign", [](PyDbSweepOptions& s, int v){
			s.impObj()->setAlign(static_cast<AcDbSweepOptions::AlignOption>(v)); return true;
		}, py::arg("v"))
		// ---- miterOption（MiterOption） ----
		.def("miterOption", [](PyDbSweepOptions& s){ return static_cast<int>(s.impObj()->miterOption()); })
		.def("setMiterOption", [](PyDbSweepOptions& s, int v){
			s.impObj()->setMiterOption(static_cast<AcDbSweepOptions::MiterOption>(v)); return true;
		}, py::arg("v"))
		// ---- alignStart ----
		.def("alignStart", [](PyDbSweepOptions& s){ return s.impObj()->alignStart(); })
		.def("setAlignStart", [](PyDbSweepOptions& s, bool v){ s.impObj()->setAlignStart(v); return true; }, py::arg("v"))
		// ---- basePoint（AcGePoint3d；SDK 签名为非 const 引用，内部拷贝） ----
		.def("basePoint", [](PyDbSweepOptions& s){ return s.impObj()->basePoint(); })
		.def("setBasePoint", [](PyDbSweepOptions& s, const AcGePoint3d& pnt){
			AcGePoint3d tmp = pnt; s.impObj()->setBasePoint(tmp); return true;
		}, py::arg("pnt"))
		// ---- bank ----
		.def("bank", [](PyDbSweepOptions& s){ return s.impObj()->bank(); })
		.def("setBank", [](PyDbSweepOptions& s, bool v){ s.impObj()->setBank(v); return true; }, py::arg("v"))
		// ---- checkIntersections ----
		.def("checkIntersections", [](PyDbSweepOptions& s){ return s.impObj()->checkIntersections(); })
		.def("setCheckIntersections", [](PyDbSweepOptions& s, bool v){ s.impObj()->setCheckIntersections(v); return true; }, py::arg("v"))
		// ---- twistRefVec（AcGeVector3d） ----
		.def("twistRefVec", [](PyDbSweepOptions& s){ return s.impObj()->twistRefVec(); })
		.def("setTwistRefVec", [](PyDbSweepOptions& s, const AcGeVector3d& vec){ s.impObj()->setTwistRefVec(vec); return true; }, py::arg("vec"))
		// ---- getSweepEntityTransform -> (ok, mat) ----
		.def("getSweepEntityTransform", [](PyDbSweepOptions& s) -> std::tuple<bool, AcGeMatrix3d> {
			AcGeMatrix3d mat;
			bool ok = s.impObj()->getSweepEntityTransform(mat);
			return { ok, mat };
		})
		// ---- setSweepEntityTransform(AcGeMatrix3d&) ----
		.def("setSweepEntityTransformByMatrix", [](PyDbSweepOptions& s, const AcGeMatrix3d& mat){
			AcGeMatrix3d tmp = mat; s.impObj()->setSweepEntityTransform(tmp); return true;
		}, py::arg("mat"))
		// ---- setSweepEntityTransform(AcArray<AcDbEntity*>&, bool) -> ErrorStatus ----
		.def("setSweepEntityTransformByEntities", [](PyDbSweepOptions& s,
			const std::vector<PyDbEntity*>& sweepEntities, bool displayErrorMessages){
			AcArray<AcDbEntity*> arr;
			for (auto e : sweepEntities) if (e) arr.append(e->impObj());
			return PyWarnIfError(s.impObj()->setSweepEntityTransform(arr, displayErrorMessages),
				L"AcDbSweepOptions::setSweepEntityTransform");
		}, py::arg("sweepEntities"), py::arg("displayErrorMessages") = false)
		// ---- getPathEntityTransform -> (ok, mat) ----
		.def("getPathEntityTransform", [](PyDbSweepOptions& s) -> std::tuple<bool, AcGeMatrix3d> {
			AcGeMatrix3d mat;
			bool ok = s.impObj()->getPathEntityTransform(mat);
			return { ok, mat };
		})
		// ---- setPathEntityTransform(AcGeMatrix3d&) ----
		.def("setPathEntityTransformByMatrix", [](PyDbSweepOptions& s, const AcGeMatrix3d& mat){
			AcGeMatrix3d tmp = mat; s.impObj()->setPathEntityTransform(tmp); return true;
		}, py::arg("mat"))
		// ---- setPathEntityTransform(AcDbEntity*, bool) -> ErrorStatus ----
		.def("setPathEntityTransformByEntity", [](PyDbSweepOptions& s, PyDbEntity& pathEnt, bool displayErrorMessages){
			return PyWarnIfError(s.impObj()->setPathEntityTransform(pathEnt.impObj(), displayErrorMessages),
				L"AcDbSweepOptions::setPathEntityTransform");
		}, py::arg("pathEnt"), py::arg("displayErrorMessages") = false)
		// ---- checkSweepCurve -> (ok, planarity, pnt, vec, closed, approxArcLen) ----
		.def("checkSweepCurve", [](PyDbSweepOptions& s, PyDbEntity& sweepEnt, bool displayErrorMessages)
			-> std::tuple<bool, int, AcGePoint3d, AcGeVector3d, bool, double> {
			AcDb::Planarity planarity = AcDb::kPlanar;
			AcGePoint3d pnt; AcGeVector3d vec;
			bool closed = false; double approxArcLen = 0.0;
			Acad::ErrorStatus es = s.impObj()->checkSweepCurve(sweepEnt.impObj(),
				planarity, pnt, vec, closed, approxArcLen, displayErrorMessages);
			bool ok = PyWarnIfError(es, L"AcDbSweepOptions::checkSweepCurve");
			return { ok, static_cast<int>(planarity), pnt, vec, closed, approxArcLen };
		}, py::arg("sweepEnt"), py::arg("displayErrorMessages") = false)
		// ---- checkPathCurve(AcDbEntity*, bool) -> bool ----
		.def("checkPathCurve", [](PyDbSweepOptions& s, PyDbEntity& pathEnt, bool displayErrorMessages){
			return PyWarnIfError(s.impObj()->checkPathCurve(pathEnt.impObj(), displayErrorMessages),
				L"AcDbSweepOptions::checkPathCurve");
		}, py::arg("pathEnt"), py::arg("displayErrorMessages") = false)
		;
}
