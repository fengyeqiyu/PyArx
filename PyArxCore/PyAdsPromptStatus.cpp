#include "stdafx.h"
#include "PyAdsPromptStatus.h"
#include <acadstrc.h>
void bind_PyPromptStatus(pym& m) {
	py::enum_<Acad::PromptStatus>(m, "PromptStatus")
		.value("eNone", Acad::eNone)
		.value("eModeless", Acad::eModeless)
		.value("eNormal", Acad::eNormal)
		.value("eError", Acad::eError)
		.value("eCancel", Acad::eCancel)
		.value("eRejected", Acad::eRejected)
		.value("eFailed", Acad::eFailed)
		.value("eKeyword", Acad::eKeyword)
		.value("eDirect", Acad::eDirect)
		.export_values();
}
