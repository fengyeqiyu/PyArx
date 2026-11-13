#pragma once
// ===========================================================================
// PyGeCurve3d.h — direct pybind11 binding for AcGeCurve3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gecurv3d.h>
#include <geent3d.h>
#include <geintrvl.h>

void bind_AcGeCurve3d(pybind11::module& m);
