#pragma once
// ===========================================================================
// PyGePointOnSurface.h — direct pybind11 binding for AcGePointOnSurface (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geponsrf.h>
#include <gevec3d.h>
#include <gevec2d.h>
#include <gesurf.h>

void bind_AcGePointOnSurface(pybind11::module& m);
