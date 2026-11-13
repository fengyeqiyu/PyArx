#pragma once
// ===========================================================================
// PyGePolyline2d.h — direct pybind11 binding for AcGePolyline2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geplin2d.h>
#include <gesent2d.h>
#include <gept2dar.h>

void bind_AcGePolyline2d(pybind11::module& m);
