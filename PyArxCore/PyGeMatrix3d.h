#pragma once
// ===========================================================================
// PyGeMatrix3d.h
// Direct pybind11 binding for SDK value type AcGeMatrix3d (4x4).
// Registered on the PyGe embedded module; no wrapper class.
// ===========================================================================
#include "stdafx.h"
#include <gegbl.h>
#include <gegblabb.h>
#include <getol.h>
#include <gepnt3d.h>
#include <gevec3d.h>
#include <geline3d.h>
#include <geplane.h>
#include <gemat3d.h>

void bind_AcGeMatrix3d(pybind11::module& m);
