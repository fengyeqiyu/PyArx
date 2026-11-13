#pragma once
// ===========================================================================
// PyRxBinder.h - PyRx 模块聚合头（umbrella）
//
// RxObject / RxClass 的绑定声明统一从这里进入。本头只做：
// 1) 汇总相关类头文件；2) 声明 bind_PyRx。bind_PyRx 定义于 PyRxBinder.cpp，
// 挂到 PyArxCore.Rx 子模块，由 PyArxBinder.cpp 调用。
// 注：PyCmd 命令管理的绑定/挂载已迁至 PyArxEngine（bind_PyCmd 由引擎调用挂
// PyArxCore.Cmd），本头不再声明；PyCmd 类（PyRxCmd）已随命令管理移出本工程。
// ===========================================================================
#include "stdafx.h"
#include "PyRxObject.h"
#include "PyRxClass.h"
#include "PyRxEnum.h"

// ---- PyRx 模块绑定声明 ----
void bind_PyRx(pym& m);
