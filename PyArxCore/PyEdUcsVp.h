// PyEdUcsVp.h - editor-level free services: UCS get/set/restore, current
// viewport scaling / circle-sides, context-menu alias, viewport queries.
// Declarations moved verbatim from PyEdBinder.h; [NEW] wrappers appended.
#pragma once

#include "stdafx.h"
#include <gemat3d.h>
#include <tuple>

// ---- moved verbatim from PyEdBinder.h ----
AcGeMatrix3d PyEdGetCurrentUCS();
bool        PyEdSetCurrentUCS(const AcGeMatrix3d& m);
bool        PyEdRestorePreviousUCS();
short       PyEdGetCurVportCircleSides();
std::tuple<double, double> PyEdCurVportPixelToDisplay();
std::tuple<double, double> PyEdCurVportScreenToDisplay();
AcString    PyEdGetCMBaseAlias(int mode);
bool        PyEdSetCMBaseAlias(const AcString& alias, int mode);

// ---- [NEW] SDK-completion wrappers (aced.h) ----
// acedSetCurVportCircleSides(short) and acedNumberOfViewports(void) are declared
// in aced.h but NOT exported from the import library (LNK2001); intentionally omitted.
uint64_t    PyEdActiveViewportId();                    // acedActiveViewportId -> oldId
uint64_t    PyEdGetCurViewportObjectId();              // acedGetCurViewportObjectId -> oldId
uint64_t    PyEdViewportIdFromNumber(int nVpNum);      // acedViewportIdFromNumber -> oldId
float       PyEdGetDpiScalingValue();                  // acedGetDpiScalingValue
