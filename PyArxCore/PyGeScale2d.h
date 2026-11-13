#pragma once
// ===========================================================================
// PyGeScale2d.h — direct pybind11 binding for AcGeScale2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gescl2d.h>

void bind_AcGeScale2d(pybind11::module& m);
