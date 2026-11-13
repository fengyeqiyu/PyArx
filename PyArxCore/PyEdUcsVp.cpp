// PyEdUcsVp.cpp - UCS / viewport scaling / context-menu alias services
// (moved verbatim from PyEd.cpp; [NEW] wrappers appended).
#include "stdafx.h"
#include "PyEdUcsVp.h"
#include "PyErrorStatusException.h"
#include "PyEdCommon.h"

// ---- Editor-level free services (UCS / viewport scaling / context-menu alias) ----
// AcEditor itself (aced.h) only exposes addReactor/removeReactor; the reachable
// high-frequency editor globals are bound below. The object-bound families
// (AcEdInputPointManager, AcEdSelectionSetService, AcEdInplaceTextEditor) need
// document/filter trampolines and are left as skeletons (see module comment).
AcGeMatrix3d PyEdGetCurrentUCS() {
    AcGeMatrix3d m;
    if (!PyWarnIfError(::acedGetCurrentUCS(m), L"acedGetCurrentUCS"))
        return AcGeMatrix3d::kIdentity; // safe default on failure
    return m;
}

bool PyEdSetCurrentUCS(const AcGeMatrix3d& m) {
    return PyWarnIfError(::acedSetCurrentUCS(m), L"acedSetCurrentUCS");
}

bool PyEdRestorePreviousUCS() {
    return PyWarnIfError(::acedRestorePreviousUCS(), L"acedRestorePreviousUCS");
}

short PyEdGetCurVportCircleSides() { return ::acedGetCurVportCircleSides(); }

std::tuple<double, double> PyEdCurVportPixelToDisplay() {
    double x = 0.0, y = 0.0;
    ::acedGetCurVportPixelToDisplay(x, y); // void SDK; defaults are safe
    return { x, y };
}

std::tuple<double, double> PyEdCurVportScreenToDisplay() {
    double x = 0.0, y = 0.0;
    ::acedGetCurVportScreenToDisplay(x, y); // void SDK; defaults are safe
    return { x, y };
}

AcString PyEdGetCMBaseAlias(int mode) {
    const ACHAR* p = ::acedGetCMBaseAlias(static_cast<AcadContextMenuMode>(mode));
    return p ? AcString(p) : AcString();
}

bool PyEdSetCMBaseAlias(const AcString& alias, int mode) {
    return ::acedSetCMBaseAlias(alias.kwszPtr(), static_cast<AcadContextMenuMode>(mode));
}

//-----------------------------------------------------------------------------
// [NEW] viewport / display queries (aced.h)
//-----------------------------------------------------------------------------
// NOTE: acedSetCurVportCircleSides(short) and acedNumberOfViewports(void) are
// declared in aced.h but NOT exported from the import library (LNK2001 on clean
// rebuild); intentionally not implemented / not bound.

uint64_t PyEdActiveViewportId() {
    AcDbObjectId id = ::acedActiveViewportId();
    return id.isNull() ? (uint64_t)0 : (uint64_t)id.asOldId();
}

uint64_t PyEdGetCurViewportObjectId() {
    AcDbObjectId id = ::acedGetCurViewportObjectId();
    return id.isNull() ? (uint64_t)0 : (uint64_t)id.asOldId();
}

uint64_t PyEdViewportIdFromNumber(int nVpNum) {
    AcDbObjectId id = ::acedViewportIdFromNumber(nVpNum);
    return id.isNull() ? (uint64_t)0 : (uint64_t)id.asOldId();
}

float PyEdGetDpiScalingValue() { return ::acedGetDpiScalingValue(); }
