#include "stdafx.h"
#include "PyGeBinder.h"
#include "PyGeShapes.h"
#include "PyGeKnotVector.h"
#include "PyGeInterval.h"

// ===========================================================================
// PyGe.cpp — module aggregator ONLY.
//
// The seven AcGe BASE value-type bindings (AcGeTol / AcGePoint2d / AcGePoint3d /
// AcGeVector2d / AcGeVector3d / AcGeMatrix2d / AcGeMatrix3d) were moved out into
// their own per-class translation units:
//   PyGeTol.cpp       -> bind_AcGeTol
//   PyGePoint2d.cpp   -> bind_AcGePoint2d
//   PyGePoint3d.cpp   -> bind_AcGePoint3d
//   PyGeVector2d.cpp  -> bind_AcGeVector2d
//   PyGeVector3d.cpp  -> bind_AcGeVector3d
//   PyGeMatrix2d.cpp  -> bind_AcGeMatrix2d
//   PyGeMatrix3d.cpp  -> bind_AcGeMatrix3d
//
// PyGe 子模块注册函数 bind_PyGe，由 PyArxBinder.cpp 挂到 PyArx.Ge。
// It wires up both the base value types below and the AcGe curve/surface/entity
// bindings declared in PyGeShapes.h (that shard is owned elsewhere and is not
// modified here).
// ===========================================================================

void bind_PyGe(pym& m) {
	m.doc() = "AutoCAD ARX AcGe Modules";
	bind_AcGeTol(m);
	bind_AcGePoint2d(m);
	bind_AcGePoint3d(m);
	bind_AcGeVector2d(m);
	bind_AcGeVector3d(m);
	bind_AcGeMatrix2d(m);
	bind_AcGeMatrix3d(m);
	bind_AcGeKnotVector(m);
	bind_AcGeInterval(m);

	// --- AcGe shapes: 2D chain (base before derived) ---
	bind_AcGeEntity2d(m);
	bind_AcGeCurve2d(m);
	bind_AcGeLinearEnt2d(m);
	bind_AcGeLine2d(m);
	bind_AcGeLineSeg2d(m);
	bind_AcGeRay2d(m);
	bind_AcGeCircArc2d(m);
	bind_AcGeSplineEnt2d(m);
	bind_AcGeNurbCurve2d(m);
	bind_AcGeOffsetCurve2d(m);
	bind_AcGeEllipArc2d(m);

	// --- AcGe shapes: 3D chain ---
	bind_AcGeEntity3d(m);
	bind_AcGeCurve3d(m);
	bind_AcGeLinearEnt3d(m);
	bind_AcGeLine3d(m);
	bind_AcGeLineSeg3d(m);
	bind_AcGeRay3d(m);
	bind_AcGeCircArc3d(m);
	bind_AcGeSplineEnt3d(m);
	bind_AcGeNurbCurve3d(m);
	bind_AcGeOffsetCurve3d(m);
	bind_AcGeEllipArc3d(m);

	// --- AcGe shapes: surfaces ---
	bind_AcGeSurface(m);
	bind_AcGePlanarEnt(m);
	bind_AcGePlane(m);
	bind_AcGeSphere(m);
	bind_AcGeCylinder(m);
	bind_AcGeCone(m);
	bind_AcGeTorus(m);

	// --- standalone ---
	bind_AcGeScale3d(m);

	// --- P1 advanced value types (bases already registered above) ---
	bind_AcGeCompositeCurve2d(m);
	bind_AcGeCompositeCurve3d(m);
	bind_AcGeCurveCurveInt2d(m);
	bind_AcGeCurveCurveInt3d(m);
	bind_AcGePointOnCurve2d(m);
	bind_AcGePointOnCurve3d(m);
	bind_AcGePointOnSurface(m);
	bind_AcGePolyline2d(m);
	bind_AcGePolyline3d(m);
	bind_AcGePosition2d(m);
	bind_AcGePosition3d(m);
	bind_AcGeScale2d(m);
}
