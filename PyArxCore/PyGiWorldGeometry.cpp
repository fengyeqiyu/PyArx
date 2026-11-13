#include "stdafx.h"
#include "PyRxObject.h"
#include "PyRxClass.h"
#include "PyGiWorldGeometry.h"
#include "PyGiSubEntityTraits.h"

PyWrapper_Cpp(GiCommonDraw);
PyWrapper_Cpp(GiWorldDraw);
PyWrapper_Cpp(GiViewportDraw);
PyWrapper_Cpp(GiGeometry);
PyWrapper_Cpp(GiWorldGeometry);
PyWrapper_Cpp(GiViewportGeometry);

// 补齐 PyGiCommonDraw / PyGiWorldDraw / PyGiViewportDraw 的方法实现（供 PyGi 绑定）
PyGiSubEntityTraits PyGiCommonDraw::subEntityTraits() const {
	return PyGiSubEntityTraits(&impObj()->subEntityTraits(), false);
}
bool PyGiCommonDraw::regenAbort() const {
	return impObj()->regenAbort() == Adesk::kTrue;
}
bool PyGiCommonDraw::isDragging() const {
	return impObj()->isDragging() == Adesk::kTrue;
}
PyGiWorldGeometry PyGiWorldDraw::geometry() const {
	return PyGiWorldGeometry(&impObj()->geometry());
}
PyGiViewportGeometry PyGiViewportDraw::geometry() const {
	return PyGiViewportGeometry(&impObj()->geometry());
}
