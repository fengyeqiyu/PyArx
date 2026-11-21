#pragma once
// ===========================================================================
// PyGeCompositeCurve2d.h — direct pybind11 binding for AcGeCompositeCurve2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gecomp2d.h>
#include <gecurv2d.h>

void bind_AcGeCompositeCurve2d(pybind11::module& m);
