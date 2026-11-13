#pragma once
// ===========================================================================
// PyGePlane.h — direct pybind11 binding for AcGePlane (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geplane.h>
#include <gelent3d.h>
#include <geplanar.h>

void bind_AcGePlane(pybind11::module& m);
