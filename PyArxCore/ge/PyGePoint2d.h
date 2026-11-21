#pragma once
// ===========================================================================
// PyGePoint2d.h
// Direct pybind11 binding for SDK value type AcGePoint2d.
// Registered on the PyGe embedded module; no wrapper class.
// ===========================================================================
#include "stdafx.h"
#include <gegbl.h>
#include <gegblabb.h>
#include <getol.h>
#include <gevec2d.h>
#include <gemat2d.h>
#include <geline2d.h>
#include <gepnt2d.h>

void bind_AcGePoint2d(pybind11::module& m);
