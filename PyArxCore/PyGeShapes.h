#pragma once
// ===========================================================================
// PyGeShapes.h
// Direct pybind11 bindings for ObjectARX AcGe curve / surface / entity classes.
// These are bound directly on the SDK types (no wrapper class), mirroring the
// style of PyGe.cpp (AcGePoint3d / AcGeVector3d / ... already registered there).
//
// Registration order contract (base before derived):
//   AcGeEntity2d -> AcGeCurve2d -> AcGeLinearEnt2d -> AcGeLine2d
//                              -> AcGeCircArc2d
//                              -> AcGeSplineEnt2d -> AcGeNurbCurve2d
//                              -> AcGeOffsetCurve2d
//                              -> AcGeEllipArc2d
//   AcGeEntity3d -> AcGeCurve3d -> AcGeLinearEnt3d -> AcGeLine3d
//                              -> AcGeCircArc3d
//                              -> AcGeSplineEnt3d -> AcGeNurbCurve3d
//                              -> AcGeOffsetCurve3d
//                              -> AcGeEllipArc3d
//                -> AcGeSurface -> AcGePlanarEnt -> AcGePlane
//                               -> AcGeSphere / AcGeCylinder / AcGeCone / AcGeTorus
//
// NOTE: AcGeTol / AcGePoint2d / AcGePoint3d / AcGeVector2d / AcGeVector3d /
//       AcGeMatrix2d / AcGeMatrix3d are ALREADY registered in PyGe.cpp and must
//       NOT be re-registered here.
// ===========================================================================
#include "stdafx.h"

// --- AcGe base / concrete headers (complete types required by py::class_) ---
#include <geent2d.h>
#include <geent3d.h>
#include <gecurv2d.h>
#include <gecurv3d.h>
#include <gelent2d.h>
#include <gelent3d.h>
#include <geline2d.h>
#include <geline3d.h>
#include <gearc2d.h>
#include <gearc3d.h>      // AcGeCircArc2d / AcGeCircArc3d (no separate AcGeCircle in ARX2024)
#include <geell2d.h>
#include <geell3d.h>
#include <gesent2d.h>     // AcGeSplineEnt2d
#include <gesent3d.h>     // AcGeSplineEnt3d
#include <genurb2d.h>
#include <genurb3d.h>
#include <geoffc2d.h>
#include <geoffc3d.h>
#include <gesurf.h>       // AcGeSurface
#include <geplanar.h>     // AcGePlanarEnt
#include <geplane.h>      // AcGePlane
#include <gecone.h>
#include <gesphere.h>
#include <gecylndr.h>
#include <getorus.h>
#include <gescl3d.h>      // AcGeScale3d
#include <gelnsg2d.h>     // AcGeLineSeg2d
#include <gelnsg3d.h>     // AcGeLineSeg3d
#include <geray2d.h>      // AcGeRay2d
#include <geray3d.h>      // AcGeRay3d
#include <geintrvl.h>     // AcGeInterval (curve get/set interval bridge)

// --- P1 advanced value types ---
#include <gecomp2d.h>     // AcGeCompositeCurve2d
#include <gecomp3d.h>     // AcGeCompositeCurve3d
#include <gecint2d.h>     // AcGeCurveCurveInt2d
#include <gecint3d.h>     // AcGeCurveCurveInt3d
#include <geponc2d.h>     // AcGePointOnCurve2d
#include <geponc3d.h>     // AcGePointOnCurve3d
#include <geponsrf.h>     // AcGePointOnSurface
#include <geplin2d.h>     // AcGePolyline2d
#include <geplin3d.h>     // AcGePolyline3d
#include <gepos2d.h>      // AcGePosition2d
#include <gepos3d.h>      // AcGePosition3d
#include <gescl2d.h>      // AcGeScale2d

// --- Per-class binding headers (split from former PyGeShapes.cpp) ---
// [2D chain]
#include "PyGeEntity2d.h"
#include "PyGeCurve2d.h"
#include "PyGeLinearEnt2d.h"
#include "PyGeLine2d.h"
#include "PyGeLineSeg2d.h"
#include "PyGeRay2d.h"
#include "PyGeCircArc2d.h"
#include "PyGeSplineEnt2d.h"
#include "PyGeNurbCurve2d.h"
#include "PyGeOffsetCurve2d.h"
#include "PyGeEllipArc2d.h"
// [3D chain]
#include "PyGeEntity3d.h"
#include "PyGeCurve3d.h"
#include "PyGeLinearEnt3d.h"
#include "PyGeLine3d.h"
#include "PyGeLineSeg3d.h"
#include "PyGeRay3d.h"
#include "PyGeCircArc3d.h"
#include "PyGeSplineEnt3d.h"
#include "PyGeNurbCurve3d.h"
#include "PyGeOffsetCurve3d.h"
#include "PyGeEllipArc3d.h"
// [Surfaces]
#include "PyGeSurface.h"
#include "PyGePlanarEnt.h"
#include "PyGePlane.h"
#include "PyGeSphere.h"
#include "PyGeCylinder.h"
#include "PyGeCone.h"
#include "PyGeTorus.h"
// [Standalone]
#include "PyGeScale3d.h"
// [P1 advanced value types]
#include "PyGeCompositeCurve2d.h"
#include "PyGeCompositeCurve3d.h"
#include "PyGeCurveCurveInt2d.h"
#include "PyGeCurveCurveInt3d.h"
#include "PyGePointOnCurve2d.h"
#include "PyGePointOnCurve3d.h"
#include "PyGePointOnSurface.h"
#include "PyGePolyline2d.h"
#include "PyGePolyline3d.h"
#include "PyGePosition2d.h"
#include "PyGePosition3d.h"
#include "PyGeScale2d.h"

// ---------------------------------------------------------------------------
// Binding macros
// ---------------------------------------------------------------------------
// Declare a per-class bind function: void bind_AcGeXxx(pybind11::module& m);
#define PYGE_DECLARE_BIND(Cls) \
	void bind_##Cls(pybind11::module& m);

// Invoke a per-class bind function inside a module block: bind_AcGeXxx(m);
#define PYGE_REGISTER(Cls) \
	bind_##Cls(m);

// ---------------------------------------------------------------------------
// Per-class bind function declarations (implemented in per-class PyGe*.cpp;
// this header is the aggregator). The matching per-class header is included above.
// Abstract base classes are registered WITHOUT a py::init; concrete classes
// expose their SDK constructors.
// ---------------------------------------------------------------------------
// 2D hierarchy
PYGE_DECLARE_BIND(AcGeEntity2d)
PYGE_DECLARE_BIND(AcGeCurve2d)
PYGE_DECLARE_BIND(AcGeLinearEnt2d)
PYGE_DECLARE_BIND(AcGeLine2d)
PYGE_DECLARE_BIND(AcGeLineSeg2d)
PYGE_DECLARE_BIND(AcGeRay2d)
PYGE_DECLARE_BIND(AcGeCircArc2d)
PYGE_DECLARE_BIND(AcGeSplineEnt2d)
PYGE_DECLARE_BIND(AcGeNurbCurve2d)
PYGE_DECLARE_BIND(AcGeOffsetCurve2d)
PYGE_DECLARE_BIND(AcGeEllipArc2d)

// 3D hierarchy
PYGE_DECLARE_BIND(AcGeEntity3d)
PYGE_DECLARE_BIND(AcGeCurve3d)
PYGE_DECLARE_BIND(AcGeLinearEnt3d)
PYGE_DECLARE_BIND(AcGeLine3d)
PYGE_DECLARE_BIND(AcGeLineSeg3d)
PYGE_DECLARE_BIND(AcGeRay3d)
PYGE_DECLARE_BIND(AcGeCircArc3d)
PYGE_DECLARE_BIND(AcGeSplineEnt3d)
PYGE_DECLARE_BIND(AcGeNurbCurve3d)
PYGE_DECLARE_BIND(AcGeOffsetCurve3d)
PYGE_DECLARE_BIND(AcGeEllipArc3d)

// Surfaces
PYGE_DECLARE_BIND(AcGeSurface)
PYGE_DECLARE_BIND(AcGePlanarEnt)
PYGE_DECLARE_BIND(AcGePlane)
PYGE_DECLARE_BIND(AcGeSphere)
PYGE_DECLARE_BIND(AcGeCylinder)
PYGE_DECLARE_BIND(AcGeCone)
PYGE_DECLARE_BIND(AcGeTorus)

// Standalone (no inheritance dependency)
PYGE_DECLARE_BIND(AcGeScale3d)

// P1 advanced value types (skeleton + high-frequency methods)
PYGE_DECLARE_BIND(AcGeCompositeCurve2d)
PYGE_DECLARE_BIND(AcGeCompositeCurve3d)
PYGE_DECLARE_BIND(AcGeCurveCurveInt2d)
PYGE_DECLARE_BIND(AcGeCurveCurveInt3d)
PYGE_DECLARE_BIND(AcGePointOnCurve2d)
PYGE_DECLARE_BIND(AcGePointOnCurve3d)
PYGE_DECLARE_BIND(AcGePointOnSurface)
PYGE_DECLARE_BIND(AcGePolyline2d)
PYGE_DECLARE_BIND(AcGePolyline3d)
PYGE_DECLARE_BIND(AcGePosition2d)
PYGE_DECLARE_BIND(AcGePosition3d)
PYGE_DECLARE_BIND(AcGeScale2d)
