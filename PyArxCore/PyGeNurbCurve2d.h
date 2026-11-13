#pragma once
// ===========================================================================
// PyGeNurbCurve2d.h — direct pybind11 binding for AcGeNurbCurve2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <genurb2d.h>
#include <gesent2d.h>
#include <gekvec.h>
#include <gept2dar.h>

void bind_AcGeNurbCurve2d(pybind11::module& m);
