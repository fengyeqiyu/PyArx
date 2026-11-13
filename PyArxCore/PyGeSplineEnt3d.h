#pragma once
// ===========================================================================
// PyGeSplineEnt3d.h — direct pybind11 binding for AcGeSplineEnt3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gesent3d.h>
#include <gecurv3d.h>

void bind_AcGeSplineEnt3d(pybind11::module& m);
