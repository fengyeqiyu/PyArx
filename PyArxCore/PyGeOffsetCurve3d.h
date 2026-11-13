#pragma once
// ===========================================================================
// PyGeOffsetCurve3d.h — direct pybind11 binding for AcGeOffsetCurve3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geoffc3d.h>
#include <gemat3d.h>
#include <gecurv3d.h>

void bind_AcGeOffsetCurve3d(pybind11::module& m);
