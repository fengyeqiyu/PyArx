// PyEdEnt.h - legacy ADS entity accessors (acdbEnt*) and the resbuf command
// chain entry point (acedCmdS). Modern database editing belongs to PyDb.
// Declarations moved verbatim from PyEdBinder.h; [NEW] wrappers appended.
#pragma once

#include "stdafx.h"
#include "PyResbuf.h"
#include "PyAdsName.h"
#include <tuple>
#include <string>

// ---- moved verbatim from PyEdBinder.h ----
PyResbuf  PyEdEntGet(const PyAdsName& ename);
bool      PyEdEntMod(const TVList& dxf);
bool      PyEdCmd(const TVList& args);

// ---- [NEW] legacy ADS entity traversal / mutation (acedads.h) ----
PyAdsName   PyEdEntNext(const PyAdsName& ename = PyAdsName{}); // 空名称 -> first entity；末尾/失败返回空 AdsName
PyAdsName   PyEdEntLast();                                  // acdbEntLast；失败返回空 AdsName
bool        PyEdEntDel(const PyAdsName& ename);             // acdbEntDel (toggle delete/undelete)
PyAdsName   PyEdHandEnt(const AcString& handle);         // acdbHandEnt；失败返回空 AdsName
PyAdsName   PyEdEntMakeX(const TVList& dxf);                 // acdbEntMakeX -> new entity name；失败返回空 AdsName
bool        PyEdEntUpd(const PyAdsName& ename);              // acdbEntUpd (redraw graphics after EntMod)
PyResbuf    PyEdVports();                                     // acedVports -> owned resbuf chain

// ---- acdbEntGetX / acdbRegApp ----
py::object  PyEdEntGetX(const PyAdsName& ename, const TVList& args);
bool        PyEdRegApp(const AcString& appname);

// ---- acdb 符号表 / 字典（返回 DXF 列表）----
py::object  PyEdTblNext(const AcString& tblname, bool rewind);
py::object  PyEdTblSearch(const AcString& tblname, const AcString& sym, bool setnext);
PyAdsName   PyEdTblObjName(const AcString& tblname, const AcString& sym);
PyAdsName   PyEdNamedObjDict();
py::object  PyEdDictSearch(const PyAdsName& dict, const AcString& sym, bool setnext);
py::object  PyEdDictNext(const PyAdsName& dict, bool rewind);
bool        PyEdDictRename(const PyAdsName& dict, const AcString& oldsym, const AcString& newsym);
bool        PyEdDictRemove(const PyAdsName& dict, const AcString& sym);
PyAdsName   PyEdDictAdd(const PyAdsName& dict, const AcString& sym, const PyAdsName& obj);
