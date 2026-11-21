#pragma once
// ===========================================================================
// PyGePosition3d.h — direct pybind11 binding for AcGePosition3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gepos3d.h>

void bind_AcGePosition3d(pybind11::module& m);
