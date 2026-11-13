#include "stdafx.h"
#include "PyGeShapes.h"

// ===========================================================================
// Aggregator TU (split completed).
// All bind_AcGe* implementations now live in per-class PyGe*.cpp files in this
// directory. The PyGe embedded module (PYBIND11_EMBEDDED_MODULE) is owned by
// PyGe.cpp, which calls these bind_* functions in base-before-derived order.
// This file intentionally carries NO module init and NO binding implementation.
// ===========================================================================
