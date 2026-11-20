#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbOleFrame.h"

PyWrapper_Cpp(DbOleFrame);

// AcDbOleFrame 的 CHM 真实方法中：
//   dwgInFields/dwgOutFields/dxfInFields/dxfOutFields —— 均为 AcDbObject 虚函数 override，
//       PyDbObject 已统一绑定（虚表分派自动落到 AcDbOleFrame 实现），不重复绑。
//   setOleObject(const void* pItem) —— OLE 内部指针，Python 侧无法安全构造，
//       这里以整数地址透传（高级用法，调用方需自行保证指针有效性）。
void bind_PyDbOleFrame(pym& m) {
	py::class_<PyDbOleFrame, PyDbEntity>(m, "PyDbOleFrame")
		.def(py::init<>())
		.def(py::init<AcDbOleFrame*>(), py::arg("p"))
		PyWrapper_BIND(DbOleFrame)
		// ---- getOleObject() -> intptr（内部 OLE 指针地址，只读） ----
		.def("getOleObject", [](const PyDbOleFrame& s) -> uint64_t {
			if (s.isNull()) return 0;
			void* p = s.impObj()->getOleObject();
			return reinterpret_cast<uint64_t>(p);
		}, "返回内部 OLE 对象指针地址（整数，只读）")
		// ---- setOleObject(addr) ----
		.def("setOleObject", [](PyDbOleFrame& s, uint64_t addr) {
			s.impObj()->setOleObject(reinterpret_cast<const void*>(addr));
		}, py::arg("addr"), "按整数地址设置 OLE 对象（高级用法，需自行保证指针有效）")
		;
}
