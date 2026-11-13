#pragma once
// ===========================================================================
// PyGeOffsetCurve2d.h — direct pybind11 binding for AcGeOffsetCurve2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geoffc2d.h>
#include <gemat2d.h>
#include <gecurv2d.h>

void bind_AcGeOffsetCurve2d(pybind11::module& m);
