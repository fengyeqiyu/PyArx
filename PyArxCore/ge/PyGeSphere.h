#pragma once
// ===========================================================================
// PyGeSphere.h — direct pybind11 binding for AcGeSphere (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <gesphere.h>
#include <gesurf.h>

void bind_AcGeSphere(pybind11::module& m);
