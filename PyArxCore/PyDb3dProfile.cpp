#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "db3dProfile.h"
#include "PyDbEntity.h"
#include "PyRxObject.h"
#include "PyDb3dProfile.h"

PyDb3dProfile::PyDb3dProfile() : m_pObj(new AcDb3dProfile()) {}
PyDb3dProfile::PyDb3dProfile(AcDb3dProfile* p) : m_pObj(p) {}
PyDb3dProfile::PyDb3dProfile(PyDbEntity& entity) : m_pObj(entity.impObj() ? new AcDb3dProfile(entity.impObj()) : nullptr) {}
PyDb3dProfile::~PyDb3dProfile() { delete m_pObj; }
AcDb3dProfile* PyDb3dProfile::impObj() { return m_pObj; }
const AcDb3dProfile* PyDb3dProfile::impObj() const { return m_pObj; }

void bind_PyDb3dProfile(pym& m) {
	py::class_<PyDb3dProfile>(m, "PyDb3dProfile")
		.def(py::init<>(), "默认构造")
		.def(py::init<PyDbEntity&>(), py::arg("entity"), "由 AcDbEntity 构造 profile")

		// ---- copyFrom(const AcRxObject*) -> bool ----
		.def("copyFrom", [](PyDb3dProfile& s, const PyRxObject& src){
			if (!s.impObj() || !src.impObj()) return false;
			return PyWarnIfError(s.impObj()->copyFrom(src.impObj()), L"AcDb3dProfile::copyFrom");
		}, py::arg("src"))

		// ---- set(AcDbEntity*) -> bool（PathRef/VertexRef 重载依赖未封装类型，跳过） ----
		.def("setByEntity", [](PyDb3dProfile& s, PyDbEntity& entity){
			if (!s.impObj() || !entity.impObj()) return false;
			return PyWarnIfError(s.impObj()->set(entity.impObj()), L"AcDb3dProfile::set(AcDbEntity*)");
		}, py::arg("entity"))

		// ---- convertProfile(4 bool) -> list[PyDb3dProfile] ----
		// 输出 AcDb3dProfile* 数组由 SDK 分配、调用方 delete；包装为 PyDb3dProfile 接管（析构 delete）。
		.def("convertProfile", [](PyDb3dProfile& s,
			bool explodeMultiFaceRegions, bool convertSurfaceToEdges,
			bool nonPlanarOnly, bool outerLoopOnly) -> std::vector<PyDb3dProfile> {
			std::vector<PyDb3dProfile> out;
			if (!s.impObj()) return out;
			AcArray<AcDb3dProfile*> arr;
			Acad::ErrorStatus es = s.impObj()->convertProfile(explodeMultiFaceRegions,
				convertSurfaceToEdges, nonPlanarOnly, outerLoopOnly, arr);
			if (!PyWarnIfError(es, L"AcDb3dProfile::convertProfile")) return out;
			for (int i = 0; i < arr.length(); ++i) {
				// 接管 SDK new 出的对象；PyDb3dProfile 析构 delete。
				out.emplace_back(arr[i]);
			}
			return out;
		}, py::arg("explodeMultiFaceRegions"), py::arg("convertSurfaceToEdges"),
			py::arg("nonPlanarOnly"), py::arg("outerLoopOnly"))

		// ---- 状态查询（SDK 头有、清单未列，补绑） ----
		.def("isClosed", [](PyDb3dProfile& s){ return s.impObj()->isClosed(); })
		.def("isPlanar", [](PyDb3dProfile& s){ return s.impObj()->isPlanar(); })
		.def("isSubent", [](PyDb3dProfile& s){ return s.impObj()->isSubent(); })
		.def("isFace", [](PyDb3dProfile& s){ return s.impObj()->isFace(); })
		.def("isEdge", [](PyDb3dProfile& s){ return s.impObj()->isEdge(); })
		.def("isValid", [](PyDb3dProfile& s){ return s.impObj()->isValid(); })
		;
}
