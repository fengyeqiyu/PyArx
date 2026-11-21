#pragma once
// ===========================================================================
// PyGeSurface.h — direct pybind11 binding for AcGeSurface (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gesurf.h>
#include <geintrvl.h>
#include <geent3d.h>

void bind_AcGeSurface(pybind11::module& m);
