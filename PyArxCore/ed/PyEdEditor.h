#pragma once
#include "stdafx.h"

// P1：对齐 PyRx Ed.Editor / Ed.EditorReactor
// Editor 走静态转发（PyArx 的 aced*/pyed* 函数已具备等价能力），EditorReactor 为占位类（事件未接线）
void bind_PyEdEditor(pym& ed);
