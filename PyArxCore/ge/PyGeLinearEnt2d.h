#pragma once
// ===========================================================================
// PyGeLinearEnt2d.h — direct pybind11 binding for AcGeLinearEnt2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gelent2d.h>
#include <gevec2d.h>
#include <gecurv2d.h>

void bind_AcGeLinearEnt2d(pybind11::module& m);
