#pragma once
// ===========================================================================
// PyGeCircArc2d.h — direct pybind11 binding for AcGeCircArc2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gearc2d.h>
#include <gevec2d.h>
#include <gecurv2d.h>

void bind_AcGeCircArc2d(pybind11::module& m);
