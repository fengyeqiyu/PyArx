#pragma once
// ===========================================================================
// PyGeEllipArc2d.h — direct pybind11 binding for AcGeEllipArc2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geell2d.h>
#include <gevec2d.h>
#include <gecurv2d.h>

void bind_AcGeEllipArc2d(pybind11::module& m);
