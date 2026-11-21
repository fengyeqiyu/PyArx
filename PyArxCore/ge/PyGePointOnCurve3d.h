#pragma once
// ===========================================================================
// PyGePointOnCurve3d.h — direct pybind11 binding for AcGePointOnCurve3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geponc3d.h>
#include <gecurv3d.h>

void bind_AcGePointOnCurve3d(pybind11::module& m);
