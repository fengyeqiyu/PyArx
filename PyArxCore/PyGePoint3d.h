#pragma once
// ===========================================================================
// PyGePoint3d.h
// Direct pybind11 binding for SDK value type AcGePoint3d (buffer protocol).
// Registered on the PyGe embedded module; no wrapper class.
// ===========================================================================
#include "stdafx.h"
#include <gegbl.h>
#include <gegblabb.h>
#include <getol.h>
#include <gevec3d.h>
#include <gemat3d.h>
#include <gepnt2d.h>
#include <geplanar.h>
#include <geplane.h>
#include <gepnt3d.h>

void bind_AcGePoint3d(pybind11::module& m);
