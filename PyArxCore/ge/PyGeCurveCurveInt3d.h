#pragma once
// ===========================================================================
// PyGeCurveCurveInt3d.h — direct pybind11 binding for AcGeCurveCurveInt3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gecint3d.h>
#include <geent3d.h>

void bind_AcGeCurveCurveInt3d(pybind11::module& m);
