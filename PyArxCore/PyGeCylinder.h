#pragma once
// ===========================================================================
// PyGeCylinder.h — direct pybind11 binding for AcGeCylinder (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gecylndr.h>
#include <gesurf.h>
#include <geintrvl.h>

void bind_AcGeCylinder(pybind11::module& m);
