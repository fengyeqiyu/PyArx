#pragma once
//-----------------------------------------------------------------------------
// PyApp.h — PyArxCore 的 Application 门面（静态类），对齐 .NET
//   Autodesk.AutoCAD.ApplicationServices.Application 的公开成员。
//
// 形态：静态门面（对齐 .NET 的 Application.DocumentManager 式直接属性访问）。
// 实现：全部落在 ObjectARX 真实 API 上：
//   DocumentManager          -> acDocManager  (AcApDocManager)
//   DocumentWindowCollection -> acDocWindowManager  (AcApDocWindowManager)
//   MainWindow / NonInPlaceMainWindow -> adsw_acadMainWnd() / acedGetAcadFrame()
//   StatusBar                -> acedGetApplicationStatusBar()  (AcApStatusBar)
//   LoadPartialMenu/UnloadPartialMenu -> acedLoadPartialMenu / acedUnloadPartialMenu
//   ReloadAllMenus           -> acedReloadMenus
//   AcadApplication(COM)     -> acedGetIDispatch
//   右键菜单扩展            -> acedAddDefaultContextMenu / acedAddObjectContextMenu
//   SetCurrentWorkspace      -> aced 命令 "-WSSET"（无直接 C++ API，经命令间接实现）
//   其余无 C++ 等价物成员（InfoCenter / Preferences / UIBindings / MenuBar /
//   MenuGroups / DisplayTextScreen / ShowModal* / DoDragDrop / InvokeHelp 等）
//   -> 抛 NotImplementedError（可查 _unsupported 说明）。
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "PyApDocManager.h"

#include <acdocman.h>
#include <AcApDocWindow.h>
#include <acedads.h>
#include <aced.h>
#include <rxmfcapi.h>
#include <core_rxmfcapi.h>
#include <AcStatusBar.h>

void bind_PyApp(pym& m);
void bind_PyDocumentWindowCollection(pym& m);
void bind_PyStatusBar(pym& m);
