#pragma once
// ===========================================================================
// PyGeRay3d.h — direct pybind11 binding for AcGeRay3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geray3d.h>
#include <gevec3d.h>
#include <gelent3d.h>

void bind_AcGeRay3d(pybind11::module& m);
