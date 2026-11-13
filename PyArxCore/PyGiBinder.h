#pragma once
#include "stdafx.h"

// PyGiBinder.h —— PyArx.Gi 子模块聚合头：统一声明 AcGi 图形接口类的 pybind 绑定函数。
// 对应 PyArx.Gi 子模块（依赖 PyRxObject/AcDbObjectId/PyDbEntity，须在 PyArx.Db 之后初始化）。

// 调用 PyArx.Gi 子模块全部公共绑定；由 bind_PyGi 统一调用。
void bind_PyGiSubEntityTraits(pym& m);
void bind_PyGiDrawableTraits(pym& m);
void bind_PyGiCommonDraw(pym& m);
void bind_PyGiWorldDraw(pym& m);
void bind_PyGiViewportDraw(pym& m);
void bind_PyGiGeometry(pym& m);
void bind_PyGiWorldGeometry(pym& m);
void bind_PyGiViewportGeometry(pym& m);
void bind_PyGiDrawable(pym& m);

// bind_PyGi: PyArx.Gi 子模块入口，由 PyArxBinder.cpp 调用。
void bind_PyGi(pym& m);
