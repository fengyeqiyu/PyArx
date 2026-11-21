#pragma once
// ===========================================================================
// PyGePointOnCurve2d.h — direct pybind11 binding for AcGePointOnCurve2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geponc2d.h>
#include <gecurv2d.h>

void bind_AcGePointOnCurve2d(pybind11::module& m);
