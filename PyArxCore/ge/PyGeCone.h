#pragma once
// ===========================================================================
// PyGeCone.h — direct pybind11 binding for AcGeCone (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gecone.h>
#include <gesurf.h>
#include <geintrvl.h>

void bind_AcGeCone(pybind11::module& m);
