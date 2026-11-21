#pragma once
// ===========================================================================
// PyGeSplineEnt2d.h — direct pybind11 binding for AcGeSplineEnt2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gesent2d.h>
#include <gecurv2d.h>

void bind_AcGeSplineEnt2d(pybind11::module& m);
