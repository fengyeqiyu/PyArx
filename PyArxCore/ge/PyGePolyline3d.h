#pragma once
// ===========================================================================
// PyGePolyline3d.h — direct pybind11 binding for AcGePolyline3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geplin3d.h>
#include <gesent3d.h>
#include <gept3dar.h>

void bind_AcGePolyline3d(pybind11::module& m);
