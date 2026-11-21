#pragma once
// ===========================================================================
// PyGeLineSeg3d.h — direct pybind11 binding for AcGeLineSeg3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gelnsg3d.h>
#include <gevec3d.h>
#include <gelent3d.h>

void bind_AcGeLineSeg3d(pybind11::module& m);
