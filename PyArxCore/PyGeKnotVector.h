#pragma once
// ===========================================================================
// PyGeKnotVector.h
// Direct pybind11 binding for SDK value type AcGeKnotVector (knot vector).
// Registered on the PyGe embedded module; no wrapper class.
// AcGeKnotVector 是 NURB 样条的节点向量（非 AcArray typedef），支持构造、
// 索引、增删、翻转、区间查询与容差管理。
// ===========================================================================
#include "stdafx.h"
#include <gegbl.h>
#include <gegblabb.h>
#include <getol.h>
#include <gekvec.h>
#include <geintrvl.h>

void bind_AcGeKnotVector(pybind11::module& m);
