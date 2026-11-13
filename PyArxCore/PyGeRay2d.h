#pragma once
// ===========================================================================
// PyGeRay2d.h — direct pybind11 binding for AcGeRay2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geray2d.h>
#include <gevec2d.h>
#include <gelent2d.h>

void bind_AcGeRay2d(pybind11::module& m);
