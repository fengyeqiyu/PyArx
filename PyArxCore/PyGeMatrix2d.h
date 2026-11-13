#pragma once
// ===========================================================================
// PyGeMatrix2d.h
// Direct pybind11 binding for SDK value type AcGeMatrix2d (3x3).
// Registered on the PyGe embedded module; no wrapper class.
// ===========================================================================
#include "stdafx.h"
#include <gegbl.h>
#include <gegblabb.h>
#include <getol.h>
#include <gepnt2d.h>
#include <gevec2d.h>
#include <geline2d.h>
#include <gemat2d.h>

void bind_AcGeMatrix2d(pybind11::module& m);
