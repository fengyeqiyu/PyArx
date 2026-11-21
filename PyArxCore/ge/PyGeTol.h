#pragma once
// ===========================================================================
// PyGeTol.h
// Direct pybind11 binding for SDK value type AcGeTol (tolerance pair).
// Registered on the PyGe embedded module; no wrapper class.
// ===========================================================================
#include "stdafx.h"
#include <gegbl.h>
#include <gegblabb.h>
#include <getol.h>

void bind_AcGeTol(pybind11::module& m);
