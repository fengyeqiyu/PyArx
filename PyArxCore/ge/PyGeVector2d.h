#pragma once
// ===========================================================================
// PyGeVector2d.h
// Direct pybind11 binding for SDK value type AcGeVector2d.
// Registered on the PyGe embedded module; no wrapper class.
// ===========================================================================
#include "stdafx.h"
#include <gegbl.h>
#include <gegblabb.h>
#include <getol.h>
#include <gemat2d.h>
#include <gevec2d.h>

void bind_AcGeVector2d(pybind11::module& m);
