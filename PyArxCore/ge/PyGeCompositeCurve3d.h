#pragma once
// ===========================================================================
// PyGeCompositeCurve3d.h — direct pybind11 binding for AcGeCompositeCurve3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gecomp3d.h>
#include <gecurv3d.h>

void bind_AcGeCompositeCurve3d(pybind11::module& m);
