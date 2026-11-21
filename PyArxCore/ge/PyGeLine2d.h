#pragma once
// ===========================================================================
// PyGeLine2d.h — direct pybind11 binding for AcGeLine2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geline2d.h>
#include <gelent2d.h>

void bind_AcGeLine2d(pybind11::module& m);
