#pragma once
// ===========================================================================
// PyGeLinearEnt3d.h — direct pybind11 binding for AcGeLinearEnt3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gelent3d.h>
#include <gevec3d.h>
#include <gecurv3d.h>

void bind_AcGeLinearEnt3d(pybind11::module& m);
