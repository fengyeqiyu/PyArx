#pragma once
// ===========================================================================
// PyGeEntity2d.h — direct pybind11 binding for AcGeEntity2d (split from PyGeShapes.cpp).
// ===========================================================================
#include "stdafx.h"
#include <geent2d.h>
#include <gevec2d.h>
#include <gemat2d.h>
#include <geline2d.h>

void bind_AcGeEntity2d(pybind11::module& m);
