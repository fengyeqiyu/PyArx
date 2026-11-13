#pragma once
// ===========================================================================
// PyGeCurveCurveInt2d.h — direct pybind11 binding for AcGeCurveCurveInt2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gecint2d.h>
#include <geent2d.h>

void bind_AcGeCurveCurveInt2d(pybind11::module& m);
