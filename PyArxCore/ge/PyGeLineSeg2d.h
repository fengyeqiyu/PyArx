#pragma once
// ===========================================================================
// PyGeLineSeg2d.h — direct pybind11 binding for AcGeLineSeg2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gelnsg2d.h>
#include <gevec2d.h>
#include <gelent2d.h>

void bind_AcGeLineSeg2d(pybind11::module& m);
