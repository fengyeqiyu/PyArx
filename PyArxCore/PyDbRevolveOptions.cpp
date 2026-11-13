#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "dbRevolveOptions.h"
#include "PyDbEntity.h"
#include "PyDbRevolveOptions.h"

PyDbRevolveOptions::PyDbRevolveOptions() : m_pObj(new AcDbRevolveOptions()) {}
PyDbRevolveOptions::PyDbRevolveOptions(AcDbRevolveOptions* p) : m_pObj(p) {}
PyDbRevolveOptions::~PyDbRevolveOptions() { delete m_pObj; }
AcDbRevolveOptions* PyDbRevolveOptions::impObj() { return m_pObj; }
const AcDbRevolveOptions* PyDbRevolveOptions::impObj() const { return m_pObj; }

void bind_PyDbRevolveOptions(pym& m) {
	py::class_<PyDbRevolveOptions>(m, "PyDbRevolveOptions")
		.def(py::init<>(), "默认构造（内部 new AcDbRevolveOptions）")
		// ---- draftAngle ----
		.def("draftAngle", [](PyDbRevolveOptions& s){ return s.impObj()->draftAngle(); })
		.def("setDraftAngle", [](PyDbRevolveOptions& s, double ang){ s.impObj()->setDraftAngle(ang); return true; }, py::arg("ang"))
		// ---- twistAngle ----
		.def("twistAngle", [](PyDbRevolveOptions& s){ return s.impObj()->twistAngle(); })
		.def("setTwistAngle", [](PyDbRevolveOptions& s, double ang){ s.impObj()->setTwistAngle(ang); return true; }, py::arg("ang"))
		// ---- closeToAxis ----
		.def("closeToAxis", [](PyDbRevolveOptions& s){ return s.impObj()->closeToAxis(); })
		.def("setCloseToAxis", [](PyDbRevolveOptions& s, bool v){ s.impObj()->setCloseToAxis(v); return true; }, py::arg("v"))
		// ---- checkRevolveCurve -> (ok, closed, endPointsOnAxis, planar) ----
		.def("checkRevolveCurve", [](PyDbRevolveOptions& s, PyDbEntity& revEnt,
			const AcGePoint3d& axisPnt, const AcGeVector3d& axisDir, bool displayErrorMessages)
			-> std::tuple<bool, bool, bool, bool> {
			bool closed = false, endPointsOnAxis = false, planar = false;
			Acad::ErrorStatus es = s.impObj()->checkRevolveCurve(revEnt.impObj(),
				axisPnt, axisDir, closed, endPointsOnAxis, planar, displayErrorMessages);
			bool ok = PyWarnIfError(es, L"AcDbRevolveOptions::checkRevolveCurve");
			return { ok, closed, endPointsOnAxis, planar };
		}, py::arg("revEnt"), py::arg("axisPnt"), py::arg("axisDir"), py::arg("displayErrorMessages") = false)
		;
}
