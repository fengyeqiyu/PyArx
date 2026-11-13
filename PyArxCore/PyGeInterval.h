#pragma once
// ===========================================================================
// PyGeInterval.h
// Direct pybind11 binding for SDK value type AcGeInterval (parameter interval).
// Registered on the PyGe embedded module; no wrapper class.
// AcGeInterval 表示参数区间（可无界/单点/有界），是曲线/样条定义域与
// KnotVector.getInterval 的输出类型。
// ===========================================================================
#include "stdafx.h"
#include <gegbl.h>
#include <gegblabb.h>
#include <getol.h>
#include <geintrvl.h>

void bind_AcGeInterval(pybind11::module& m);
