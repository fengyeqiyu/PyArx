#pragma once
// ===========================================================================
// PyGeTorus.h — direct pybind11 binding for AcGeTorus (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <getorus.h>
#include <gesurf.h>

void bind_AcGeTorus(pybind11::module& m);
