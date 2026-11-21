#pragma once
// ===========================================================================
// PyGeScale3d.h — direct pybind11 binding for AcGeScale3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gescl3d.h>

void bind_AcGeScale3d(pybind11::module& m);
