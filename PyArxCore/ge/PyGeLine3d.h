#pragma once
// ===========================================================================
// PyGeLine3d.h — direct pybind11 binding for AcGeLine3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geline3d.h>
#include <gelent3d.h>

void bind_AcGeLine3d(pybind11::module& m);
