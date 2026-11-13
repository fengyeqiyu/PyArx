// PyEdBinder.h - umbrella header for the AutoCAD aced* editor global functions.
// After the PyEdFuncs split, declarations live in per-family slices; this header
// re-exposes them unchanged so existing consumers (PySelfTest.cpp) compile
// without edits. Python-visible symbol/function names are untouched.
//
// Conventions (project-wide):
//   - input strings are AcString (UNICODE build, kwszPtr() -> const ACHAR*);
//   - output params are converted to return values: ads_point -> tuple(d,d,d),
//     ads_name -> tuple(int64,int64), char*/wchar_t* out -> AcString;
//   - aced* return int status codes: RTNORM(5100) means success and the result
//     value is returned; on RTCAN/RTERROR/RTREJ/RTFAIL/RTKWORD the call prints a
//     [PyArx] warning to the CAD command line and returns a safe default / false
//     (see edWarn in PyEdCommon.h); parameter-validation errors still throw.
// NOTE: interactive acedGet* / acedEntSel / acedSSGet wrap CAD command context
//       and block for user input - they are bound but MUST NOT be called in
//       self-tests.
#pragma once

#include "PyEdResult.h"          // PromptStatus / OptionFlags / Result<T> / EntityValue / SubEntityValue
#include "PyEdPromptOptions.h"  // PromptOptions
#include "PyEdFunc.h"            // non-interactive session/var/env/menu/ret family
#include "PyEdUcsVp.h"          // UCS / viewport scaling / context-menu alias
#include "PyEdSel.h"            // thin acedSS* wrappers
#include "PyEdEnt.h"            // legacy acdbEnt* + acedCmdS chain
#include "PyEdUtil.h"           // acedTrans / acdbInters / RToS / AngToS / DisToF / AngToF
#include "PyEdPrompt.h"         // 统一交互提示封装（合并原 PyEdAds/PyEdPrompts/PyEdPromptInfra）

// bind_PyEd: PyArx.Ed 子模块入口，由 PyArxBinder.cpp 调用。
void bind_PyEd(pym& m);
