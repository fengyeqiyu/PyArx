#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbPlaceHolder.h"

PyWrapper_Cpp(DbPlaceHolder);

// AcDbPlaceHolder 的全部 CHM 真实方法（dxfInFields/dxfOutFields/subWblockClone）
// 均为 AcDbObject 虚函数 override，PyDbObject 已统一绑定并通过 C++ 虚表分派到本类实现，
// 此处不再重复绑定。
void bind_PyDbPlaceHolder(pym& m) {
	py::class_<PyDbPlaceHolder, PyDbObject>(m, "PyDbPlaceHolder")
		.def(py::init<>())
		.def(py::init<AcDbPlaceHolder*>(), py::arg("p"))
		PyWrapper_BIND(DbPlaceHolder)
		;
}
