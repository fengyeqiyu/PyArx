// PyEdFunc.h - non-interactive editor/session services: prompt/alert, system
// variables, environment/config, support-path lookup, initget break flags,
// menu/ARX load, mspace/pspace, return-value family, defun/menucmd.
// Declarations moved verbatim from PyEdBinder.h; new SDK-completion wrappers
// are marked [NEW].
#pragma once

#include "stdafx.h"
#include "PyResbuf.h"
#include "PyAdsName.h"
#include <optional>

// ---- moved verbatim from PyEdBinder.h ----
bool        PyEdPrompt(const AcString& s);
bool        PyEdAlert(const AcString& s);
py::object  PyEdGetVar(const AcString& sym);
bool        PyEdSetVar(const AcString& sym, const TypedValue& value);
AcString    PyEdGetEnv(const AcString& sym);
bool        PyEdSetEnv(const AcString& sym, const AcString& val);
AcString    PyEdFindFile(const AcString& fname);
AcString    PyEdFindTrustedFile(const AcString& fname);      // 在受信任路径搜索文件
bool        PyEdInitGet(int val, const AcString& kwl);
bool        PyEdUsrBrk();
int         PyEdIsDragging();
bool        PyEdIsUpdateDisplayPaused();
void        PyEdUpdateDisplayPause(bool bEnable);
bool        PyEdFiberWorld();
bool        PyEdLoadPartialMenu(const AcString& file);
int         PyEdGetCommandVersion();
bool        PyEdMspace();
bool        PyEdPspace();
bool        PyEdSetCurrentVPort(int vpnumber);
intptr_t    PyEdRegCmdsPtr();
bool        PyEdArxLoad(const AcString& app);
bool        PyEdArxUnload(const AcString& app);
void        PyEdRetStr(const AcString& s);
void        PyEdRetInt(int v);
void        PyEdRetReal(double v);
void        PyEdRetT();
void        PyEdRetNil();
void        PyEdRetVoid();
bool        PyEdDefun(const AcString& name, int fcode);
bool        PyEdMenucmd(const AcString& s);

// ---- [NEW] SDK-completion wrappers (acedads.h / aced.h) ----
AcString    PyEdGetInput();                              // acedGetInput(AcString&)
AcString    PyEdGetAppName();                            // acedGetAppName
AcString    PyEdGetCName(const AcString& cmd);           // acedGetCName(AcString&)
AcString    PyEdGetCfg(const AcString& sym);              // acedGetCfg
bool        PyEdSetCfg(const AcString& sym, const AcString& val); // acedSetCfg
bool        PyEdUndef(const AcString& name, int fcode);            // acedUndef
int         PyEdGetFunCode();                             // acedGetFunCode
py::list    PyEdArxLoaded();                              // acedArxLoaded -> list[str]
bool        PyEdIsMenuGroupLoaded(const AcString& group);        // acedIsMenuGroupLoaded
bool        PyEdUnloadPartialMenu(const AcString& file);           // acedUnloadPartialMenu
void        PyEdReloadMenus(bool incremental);            // acedReloadMenus
void        PyEdPostCommandPrompt();                      // acedPostCommandPrompt
void        PyEdUpdateDisplay();                          // acedUpdateDisplay
bool        PyEdIsInBackgroundMode();                      // acedIsInBackgroundMode
void        PyEdEnableUsrbrk();                           // acedEnableUsrbrk
void        PyEdDisableUsrbrk();                          // acedDisableUsrbrk
bool        PyEdIsUsrbrkDisabled();                       // acedIsUsrbrkDisabled
bool        PyEdGraphScr();                               // acedGraphScr
bool        PyEdTextScr();                                // acedTextScr
bool        PyEdTextPage();                               // acedTextPage
bool        PyEdRedraw(const std::optional<PyAdsName>& ename, int mode); // acedRedraw (None/empty = whole screen)
bool        PyEdRetPoint(const AcGePoint3d& pt);            // acedRetPoint
bool        PyEdRetName(const PyAdsName& ename, int type); // acedRetName
bool        PyEdRetList(const PyResbuf& rbuf);            // acedRetList
bool        PyEdRetVal(const PyResbuf& rbuf);             // acedRetVal
bool        PyEdWcMatch(const AcString& text, const AcString& pattern); // acutWcMatch
AcString    PyEdGetFileD(const AcString& title, const AcString& defName,
    const AcString& ext, int flags); // acedGetFileD 文件对话框，取消返回空串
py::object  PyEdGetSym(const AcString& name);          // acedGetSym 读 LISP 符号，失败 None
bool        PyEdPutSym(const AcString& name, const TypedValue& value); // acedPutSym 写符号（TypedValue: code=RTSHORT/RTREAL/RTSTR…）
bool        PyEdHelp(const AcString& file, const AcString& topic, int cmd); // acedHelp
bool        PyEdHelpForExternal(const AcString& funcName);          // acedHelpForExternal
py::tuple   PyEdFNSplit(const AcString& path);                     // acedFNSplit -> (目录,文件名,扩展名)
py::object  PyEdInvoke(const TVList& args);                         // acedInvoke 调 LISP，首项函数名(RTSTR)，失败 None
bool        PyEdUpdate(const std::optional<AcGePoint3d>& p1, const std::optional<AcGePoint3d>& p2);       // acedUpdate 重生成区域，None=全视口
