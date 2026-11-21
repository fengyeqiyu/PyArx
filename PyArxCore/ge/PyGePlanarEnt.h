#pragma once
// ===========================================================================
// PyGePlanarEnt.h — direct pybind11 binding for AcGePlanarEnt (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geplanar.h>
#include <geplane.h>
#include <gelent3d.h>
#include <gesurf.h>

void bind_AcGePlanarEnt(pybind11::module& m);
