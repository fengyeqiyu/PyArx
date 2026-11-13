#pragma once
// ===========================================================================
// PyGeCurve2d.h — direct pybind11 binding for AcGeCurve2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gecurv2d.h>
#include <geent2d.h>
#include <geintrvl.h>

void bind_AcGeCurve2d(pybind11::module& m);
