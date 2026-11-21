#pragma once
// ===========================================================================
// PyGePosition2d.h — direct pybind11 binding for AcGePosition2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gepos2d.h>

void bind_AcGePosition2d(pybind11::module& m);
