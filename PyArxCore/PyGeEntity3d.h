#pragma once
// ===========================================================================
// PyGeEntity3d.h — direct pybind11 binding for AcGeEntity3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geent3d.h>
#include <gevec3d.h>
#include <gemat3d.h>
#include <geplane.h>

void bind_AcGeEntity3d(pybind11::module& m);
