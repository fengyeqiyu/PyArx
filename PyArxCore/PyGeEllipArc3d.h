#pragma once
// ===========================================================================
// PyGeEllipArc3d.h — direct pybind11 binding for AcGeEllipArc3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geell3d.h>
#include <gevec3d.h>
#include <gecurv3d.h>

void bind_AcGeEllipArc3d(pybind11::module& m);
