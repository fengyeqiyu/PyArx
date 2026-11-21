#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyRxClass.h"
#include "PyDbObjectContext.h"

PyWrapper_Cpp(DbObjectContext);

void bind_PyDbObjectContext(pym& m) {
	py::class_<PyDbObjectContext, PyRxObject>(m, "PyDbObjectContext")
		.def(py::init<AcDbObjectContext*, bool>(), py::arg("p") = nullptr, py::arg("autoDelete") = false)
		PyWrapper_BIND(DbObjectContext)
		// ---- getName(AcString&) ----
		.def("getName", [](PyDbObjectContext& s) -> AcString {
			AcString out;
      if (s.isNull()) return AcString();
			if (!PyWarnIfError(s.impObj()->getName(out), L"AcDbObjectContext::getName"))
				return AcString();
			return out;
		})
		// ---- setName(const AcString&) -> bool ----
		.def("setName", [](PyDbObjectContext& s, const AcString& v) {
      if (s.isNull()) return false;
			return PyWarnIfError(s.impObj()->setName(v.kwszPtr()), L"AcDbObjectContext::setName");
		}, py::arg("name"))
		// ---- uniqueIdentifier() -> intptr (Adesk::LongPtr) ----
		.def("uniqueIdentifier", [](PyDbObjectContext& s) -> intptr_t {
      if (s.isNull()) return 0;
			return static_cast<intptr_t>(s.impObj()->uniqueIdentifier());
		})
		// ---- collectionName()（纯虚，无 ErrorStatus，由具体派生类实现） ----
		.def("collectionName", [](PyDbObjectContext& s) -> AcString {
      if (s.isNull()) return AcString();
			return s.impObj()->collectionName();
		})
		;
	// 跳过：
	//  - clients / pack：CHM 无页面，疑误提取，跳过。
}
