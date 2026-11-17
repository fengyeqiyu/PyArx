#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "dbBlendOptions.h"
#include "PyDbBlendOptions.h"

PyDbBlendOptions::PyDbBlendOptions() : m_pObj(new AcDbBlendOptions()) {}
PyDbBlendOptions::PyDbBlendOptions(AcDbBlendOptions* p) : m_pObj(p) {}
PyDbBlendOptions::~PyDbBlendOptions() { delete m_pObj; }
AcDbBlendOptions* PyDbBlendOptions::impObj() { return m_pObj; }
const AcDbBlendOptions* PyDbBlendOptions::impObj() const { return m_pObj; }

void bind_PyDbBlendOptions(pym& m) {
	py::class_<PyDbBlendOptions>(m, "PyDbBlendOptions")
		.def(py::init<>(), "默认构造（内部 new AcDbBlendOptions）")
		// ---- simplify ----
		.def("simplify", [](PyDbBlendOptions& s){ return s.impObj() ? s.impObj()->simplify() : false; })
		.def("setSimplify", [](PyDbBlendOptions& s, bool v){
			if (!s.impObj()) return false;
			return PyWarnIfError(s.impObj()->setSimplify(v), L"AcDbBlendOptions::setSimplify");
		}, py::arg("v"))
		// ---- solid ----
		.def("solid", [](PyDbBlendOptions& s){ return s.impObj() ? s.impObj()->solid() : false; })
		.def("setSolid", [](PyDbBlendOptions& s, bool v){
			if (!s.impObj()) return false;
			return PyWarnIfError(s.impObj()->setSolid(v), L"AcDbBlendOptions::setSolid");
		}, py::arg("v"))
		// ---- quality ----
		.def("quality", [](PyDbBlendOptions& s){ return s.impObj() ? s.impObj()->quality() : 0u; })
		.def("setQuality", [](PyDbBlendOptions& s, unsigned int v){
			if (!s.impObj()) return false;
			return PyWarnIfError(s.impObj()->setQuality(v), L"AcDbBlendOptions::setQuality");
		}, py::arg("v"))
		;
}
