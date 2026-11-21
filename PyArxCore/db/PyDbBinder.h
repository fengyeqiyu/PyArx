#pragma once
// ===========================================================================
// PyDbBinder.h - PyDb 模块聚合头（umbrella）
//
// PyDb 模块（数据库实体 / 表 / 记录 / 迭代器 / 样式等全部 Db 类绑定）的
// 统一入口。本头做：1) 汇总 PyDb 相关类头文件；2) 声明模块初始化入口
// pybind11_init_PyDb（由 PyDbBinder.cpp 定义）。绑定实现仍在 PyDbBinder.cpp。
// ===========================================================================
#include "stdafx.h"
#include "PyRxBinder.h"          // RxObject/RxClass 已独立为 PyArxCore.Rx 子模块（PyCmd 挂载已迁 PyArxEngine）

// ---- PyDb 模块初始化入口（定义于 PyDbBinder.cpp） ----
void pybind11_init_PyDb(::pybind11::module_& m);
