#pragma once
// ===========================================================================
// PyGe.h
// Aggregator header for the PyGe embedded module.
//
// The seven AcGe BASE value types were split out of the former monolithic
// PyGe.cpp into one .h/.cpp pair per class (see includes below). This header
// remains the single entry point: every consumer that does `#include "PyGeBinder.h"`
// still sees the complete type set and all bind_AcGeXxx declarations unchanged,
// so existing call sites are not broken.
//
// AcGe curve / surface / entity bindings (PyGeShapes) are declared separately in
// PyGeShapes.h and are NOT re-declared here.
// ===========================================================================
#include "stdafx.h"
#include "adesk.h"
#include "gegbl.h"
#include "gegblabb.h"
#include <acdbabb.h>

// --- Per-class base value-type bindings (split from legacy PyGe.cpp) ---
#include "PyGeTol.h"
#include "PyGePoint2d.h"
#include "PyGePoint3d.h"
#include "PyGeVector2d.h"
#include "PyGeVector3d.h"
#include "PyGeMatrix2d.h"
#include "PyGeMatrix3d.h"

// --- Bind function declarations (signatures unchanged from legacy PyGe.h) ---
void bind_AcGeTol(pybind11::module& m);
void bind_AcGePoint2d(pybind11::module& m);
void bind_AcGePoint3d(pybind11::module& m);
void bind_AcGeVector2d(pybind11::module& m);
void bind_AcGeVector3d(pybind11::module& m);
void bind_AcGeMatrix2d(pybind11::module& m);
void bind_AcGeMatrix3d(pybind11::module& m);




//BindClass(AcGePlanarEnt);
//BindClass(AcGePlane);
//BindClass(AcGeLine2d);
//BindClass(AcGeBoundBlock2d);
//BindClass(AcGeBoundBlock3d);
//BindClass(AcGeLinearEnt2d);
//BindClass(AcGeLinearEnt3d);
//BindClass(AcGeEntity2d);
//BindClass(AcGeEntity3d);
//BindClass(AcGeCurve2d);
//BindClass(AcGeCurve3d);

// PyGe 子模块入口，由 PyArxBinder.cpp 调用
void bind_PyGe(pym& m);
