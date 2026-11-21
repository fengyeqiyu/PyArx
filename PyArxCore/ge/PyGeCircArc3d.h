#pragma once
// ===========================================================================
// PyGeCircArc3d.h — direct pybind11 binding for AcGeCircArc3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gearc3d.h>
#include <gevec3d.h>
#include <gecurv3d.h>

void bind_AcGeCircArc3d(pybind11::module& m);
