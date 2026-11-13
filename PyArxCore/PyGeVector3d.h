#pragma once
// ===========================================================================
// PyGeVector3d.h
// Direct pybind11 binding for SDK value type AcGeVector3d.
// Registered on the PyGe embedded module; no wrapper class.
// ===========================================================================
#include "stdafx.h"
#include <gegbl.h>
#include <gegblabb.h>
#include <getol.h>
#include <gemat3d.h>
#include <gepnt2d.h>
#include <geplanar.h>
#include <gevec3d.h>

void bind_AcGeVector3d(pybind11::module& m);
