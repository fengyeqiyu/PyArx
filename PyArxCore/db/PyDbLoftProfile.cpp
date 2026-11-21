#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "db3dProfile.h"
#include "gepnt3d.h"
#include "PyDbEntity.h"
#include "PyRxObject.h"
#include "PyDbLoftProfile.h"

PyDbLoftProfile::PyDbLoftProfile() : PyDb3dProfile(new AcDbLoftProfile()) {}
PyDbLoftProfile::PyDbLoftProfile(AcDbLoftProfile* p) : PyDb3dProfile(p) {}
PyDbLoftProfile::PyDbLoftProfile(PyDbEntity& entity) : PyDb3dProfile(new AcDbLoftProfile(entity.impObj())) {}
PyDbLoftProfile::PyDbLoftProfile(const AcGePoint3d& pnt) : PyDb3dProfile(new AcDbLoftProfile(pnt)) {}

void bind_PyDbLoftProfile(pym& m) {
	py::class_<PyDbLoftProfile, PyDb3dProfile>(m, "PyDbLoftProfile")
		.def(py::init<>(), "默认构造")
		.def(py::init<PyDbEntity&>(), py::arg("entity"), "由 AcDbEntity 构造 loft profile")
		.def(py::init<const AcGePoint3d&>(), py::arg("pnt"), "由点构造 loft profile（顶点 profile）")

		// ---- copyFrom(const AcRxObject*) -> bool ----
		.def("copyFrom", [](PyDbLoftProfile& s, const PyRxObject& src){
			if (!s.impObj() || !src.impObj()) return false;
			return PyWarnIfError(s.impObj()->copyFrom(src.impObj()), L"AcDbLoftProfile::copyFrom");
		}, py::arg("src"))

		// ---- continuity（0=G0,1=G1,2=G2） ----
		// impObj() 返回基类 AcDb3dProfile*；continuity/setContinuity 是 AcDbLoftProfile
		// 派生类新增的非虚方法（db3dProfile.h:387/396），必须向下转型访问。
		.def("continuity", [](PyDbLoftProfile& s){
			if (!s.impObj()) return 0;
			auto p = static_cast<AcDbLoftProfile*>(s.impObj());
			return static_cast<int>(p->continuity());
		})
		.def("setContinuity", [](PyDbLoftProfile& s, int v){
			if (!s.impObj()) return false;
			auto p = static_cast<AcDbLoftProfile*>(s.impObj());
			p->setContinuity(static_cast<Adesk::Int16>(v)); return true;
		}, py::arg("v"))

		// ---- magnitude ----
		.def("magnitude", [](PyDbLoftProfile& s){
			if (!s.impObj()) return 0.0;
			auto p = static_cast<AcDbLoftProfile*>(s.impObj());
			return p->magnitude();
		})
		.def("setMagnitude", [](PyDbLoftProfile& s, double v){
			if (!s.impObj()) return false;
			auto p = static_cast<AcDbLoftProfile*>(s.impObj());
			p->setMagnitude(v); return true;
		}, py::arg("v"))
		;
}
