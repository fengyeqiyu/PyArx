#pragma once
// ===========================================================================
// PyGeNurbCurve3d.h — direct pybind11 binding for AcGeNurbCurve3d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <genurb3d.h>
#include <gesent3d.h>
#include <gekvec.h>
#include <gept3dar.h>

void bind_AcGeNurbCurve3d(pybind11::module& m);
