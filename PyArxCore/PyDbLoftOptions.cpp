#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "dbLoftOptions.h"
#include "PyDbEntity.h"
#include "PyDbLoftOptions.h"

PyDbLoftOptions::PyDbLoftOptions() : m_pObj(new AcDbLoftOptions()) {}
PyDbLoftOptions::PyDbLoftOptions(AcDbLoftOptions* p) : m_pObj(p) {}
PyDbLoftOptions::~PyDbLoftOptions() { delete m_pObj; }
AcDbLoftOptions* PyDbLoftOptions::impObj() { return m_pObj; }
const AcDbLoftOptions* PyDbLoftOptions::impObj() const { return m_pObj; }

void bind_PyDbLoftOptions(pym& m) {
	// NormalOption 枚举已迁移至 PyArx.Enum 模块（LoftNormalOption）
	py::class_<PyDbLoftOptions>(m, "PyDbLoftOptions")
		.def(py::init<>(), "默认构造（内部 new AcDbLoftOptions）")
		// ---- NormalOption 枚举 ----
		.def_property_readonly_static("kNoNormal", [](py::object){ return AcDbLoftOptions::kNoNormal; })
		.def_property_readonly_static("kFirstNormal", [](py::object){ return AcDbLoftOptions::kFirstNormal; })
		.def_property_readonly_static("kLastNormal", [](py::object){ return AcDbLoftOptions::kLastNormal; })
		.def_property_readonly_static("kEndsNormal", [](py::object){ return AcDbLoftOptions::kEndsNormal; })
		.def_property_readonly_static("kAllNormal", [](py::object){ return AcDbLoftOptions::kAllNormal; })
		.def_property_readonly_static("kUseDraftAngles", [](py::object){ return AcDbLoftOptions::kUseDraftAngles; })

		// ---- draftStart / draftEnd（double 角度） ----
		.def("draftStart", [](PyDbLoftOptions& s){ return s.impObj()->draftStart(); })
		.def("setDraftStart", [](PyDbLoftOptions& s, double ang){ s.impObj()->setDraftStart(ang); return true; }, py::arg("ang"))
		.def("draftEnd", [](PyDbLoftOptions& s){ return s.impObj()->draftEnd(); })
		.def("setDraftEnd", [](PyDbLoftOptions& s, double ang){ s.impObj()->setDraftEnd(ang); return true; }, py::arg("ang"))
		// ---- draftStartMag / draftEndMag ----
		.def("draftStartMag", [](PyDbLoftOptions& s){ return s.impObj()->draftStartMag(); })
		.def("setDraftStartMag", [](PyDbLoftOptions& s, double v){ s.impObj()->setDraftStartMag(v); return true; }, py::arg("v"))
		.def("draftEndMag", [](PyDbLoftOptions& s){ return s.impObj()->draftEndMag(); })
		.def("setDraftEndMag", [](PyDbLoftOptions& s, double v){ s.impObj()->setDraftEndMag(v); return true; }, py::arg("v"))
		// ---- arcLengthParam ----
		.def("arcLengthParam", [](PyDbLoftOptions& s){ return s.impObj()->arcLengthParam(); })
		.def("setArcLengthParam", [](PyDbLoftOptions& s, bool v){ s.impObj()->setArcLengthParam(v); return true; }, py::arg("v"))
		// ---- noTwist ----
		.def("noTwist", [](PyDbLoftOptions& s){ return s.impObj()->noTwist(); })
		.def("setNoTwist", [](PyDbLoftOptions& s, bool v){ s.impObj()->setNoTwist(v); return true; }, py::arg("v"))
		// ---- alignDirection ----
		.def("alignDirection", [](PyDbLoftOptions& s){ return s.impObj()->alignDirection(); })
		.def("setAlignDirection", [](PyDbLoftOptions& s, bool v){ s.impObj()->setAlignDirection(v); return true; }, py::arg("v"))
		// ---- simplify ----
		.def("simplify", [](PyDbLoftOptions& s){ return s.impObj()->simplify(); })
		.def("setSimplify", [](PyDbLoftOptions& s, bool v){ s.impObj()->setSimplify(v); return true; }, py::arg("v"))
		// ---- closed ----
		.def("closed", [](PyDbLoftOptions& s){ return s.impObj()->closed(); })
		.def("setClosed", [](PyDbLoftOptions& s, bool v){ s.impObj()->setClosed(v); return true; }, py::arg("v"))
		// ---- ruled ----
		.def("ruled", [](PyDbLoftOptions& s){ return s.impObj()->ruled(); })
		.def("setRuled", [](PyDbLoftOptions& s, bool v){ s.impObj()->setRuled(v); return true; }, py::arg("v"))
		// ---- virtualGuide ----
		.def("virtualGuide", [](PyDbLoftOptions& s){ return s.impObj()->virtualGuide(); })
		.def("setVirtualGuide", [](PyDbLoftOptions& s, bool v){ s.impObj()->setVirtualGuide(v); return true; }, py::arg("v"))
		// ---- normal（NormalOption） ----
		.def("normal", [](PyDbLoftOptions& s){ return static_cast<int>(s.impObj()->normal()); })
		.def("setNormal", [](PyDbLoftOptions& s, int v){
			s.impObj()->setNormal(static_cast<AcDbLoftOptions::NormalOption>(v)); return true;
		}, py::arg("v"))
		// ---- sysvars 往返（返回 ErrorStatus） ----
		.def("setOptionsFromSysvars", [](PyDbLoftOptions& s){
			return PyWarnIfError(s.impObj()->setOptionsFromSysvars(), L"AcDbLoftOptions::setOptionsFromSysvars");
		})
		.def("setSysvarsFromOptions", [](PyDbLoftOptions& s){
			return PyWarnIfError(s.impObj()->setSysvarsFromOptions(), L"AcDbLoftOptions::setSysvarsFromOptions");
		})
		// ---- checkOptions ----
		.def("checkOptions", [](PyDbLoftOptions& s, bool displayErrorMessages){
			return PyWarnIfError(s.impObj()->checkOptions(displayErrorMessages), L"AcDbLoftOptions::checkOptions");
		}, py::arg("displayErrorMessages") = false)
		// ---- checkPathCurve(AcDbEntity*, bool) -> bool ----
		.def("checkPathCurve", [](PyDbLoftOptions& s, PyDbEntity& pathEnt, bool displayErrorMessages){
			return PyWarnIfError(s.impObj()->checkPathCurve(pathEnt.impObj(), displayErrorMessages),
				L"AcDbLoftOptions::checkPathCurve");
		}, py::arg("pathEnt"), py::arg("displayErrorMessages") = false)
		// ---- checkCrossSectionCurves -> (ok, allOpen, allClosed, allPlanar) ----
		.def("checkCrossSectionCurves", [](PyDbLoftOptions& s, const std::vector<PyDbEntity*>& curves, bool displayErrorMessages)
			-> std::tuple<bool, bool, bool, bool> {
			AcArray<AcDbEntity*> arr;
			for (auto e : curves) if (e) arr.append(e->impObj());
			bool allOpen = false, allClosed = false, allPlanar = false;
			Acad::ErrorStatus es = s.impObj()->checkCrossSectionCurves(arr, allOpen, allClosed, allPlanar, displayErrorMessages);
			bool ok = PyWarnIfError(es, L"AcDbLoftOptions::checkCrossSectionCurves");
			return { ok, allOpen, allClosed, allPlanar };
		}, py::arg("curves"), py::arg("displayErrorMessages") = false)
		// ---- checkGuideCurves -> bool ----
		.def("checkGuideCurves", [](PyDbLoftOptions& s, const std::vector<PyDbEntity*>& guideCurves, bool displayErrorMessages){
			AcArray<AcDbEntity*> arr;
			for (auto e : guideCurves) if (e) arr.append(e->impObj());
			return PyWarnIfError(s.impObj()->checkGuideCurves(arr, displayErrorMessages),
				L"AcDbLoftOptions::checkGuideCurves");
		}, py::arg("guideCurves"), py::arg("displayErrorMessages") = false)
		// ---- checkLoftCurves -> (ok, allOpen, allClosed, allPlanar) ----
		.def("checkLoftCurves", [](PyDbLoftOptions& s,
			const std::vector<PyDbEntity*>& crossSectionCurves,
			const std::vector<PyDbEntity*>& guideCurves,
			PyDbEntity* pPathCurve, bool displayErrorMessages) -> std::tuple<bool, bool, bool, bool> {
			AcArray<AcDbEntity*> xs;
			for (auto e : crossSectionCurves) if (e) xs.append(e->impObj());
			AcArray<AcDbEntity*> gs;
			for (auto e : guideCurves) if (e) gs.append(e->impObj());
			bool allOpen = false, allClosed = false, allPlanar = false;
			Acad::ErrorStatus es = s.impObj()->checkLoftCurves(xs, gs,
				pPathCurve ? pPathCurve->impObj() : nullptr, allOpen, allClosed, allPlanar, displayErrorMessages);
			bool ok = PyWarnIfError(es, L"AcDbLoftOptions::checkLoftCurves");
			return { ok, allOpen, allClosed, allPlanar };
		}, py::arg("crossSectionCurves"), py::arg("guideCurves"), py::arg("pPathCurve") = nullptr, py::arg("displayErrorMessages") = false)
		;
}
