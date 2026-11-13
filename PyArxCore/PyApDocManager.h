#pragma once
//-----------------------------------------------------------------------------
// PyApDocManager.h — PyArxCore 的 DocumentManager 包装，对齐 .NET
//   Autodesk.AutoCAD.ApplicationServices.DocumentManager。
//
// 对齐成员（C# -> ObjectARX）：
//   Count               -> documentCount()
//   MdiActiveDocument   -> mdiActiveDocument()（返回 PyApDocument）
//   CurDocument         -> curDocument()
//   IsApplicationContext-> isApplicationContext()
//   GetDocument(db)     -> document(const AcDbDatabase*)
//   LockDocument        -> lockDocument()
//   UnlockDocument      -> unlockDocument()
//   SetCurrentDocument  -> setCurDocument()
//   ActivateDocument    -> activateDocument()
//   Open/New/Recover    -> appContextOpenDocument / appContextNewDocument /
//                          appContextRecoverDocument
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <acdocman.h>
#include "PyApDocument.h"

class PyApDocManager {
    AcApDocManager* m_ptr = nullptr;
public:
    explicit PyApDocManager(AcApDocManager* p = nullptr) : m_ptr(p) {}
    AcApDocManager* impObj() const { return m_ptr; }

    // C# DocumentManager 对齐
    int count() const;
    bool isApplicationContext() const;
    PyApDocument mdiActiveDocument() const;
    PyApDocument curDocument() const;
    PyApDocument getDocument(const PyDbDatabase& db) const;

    std::wstring lockDocument(PyApDocument doc);
    std::wstring unlockDocument(PyApDocument doc);
    std::wstring setCurrentDocument(PyApDocument doc, bool activate);
    std::wstring activateDocument(PyApDocument doc);
    std::wstring sendStringToExecute(PyApDocument doc, const std::wstring& command,
                                     bool activate, bool wrapUpInactiveDoc, bool echoCommand);

    std::wstring openDocument(const std::wstring& path);
    std::wstring newDocument(const std::wstring& templateName);

    AcString toString() const;
};

void bind_PyApDocManager(pym& m);
