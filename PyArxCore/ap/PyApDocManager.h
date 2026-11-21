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
#include <vector>
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

    // 本文档管理器扩展（MCP / 自动化用）
    //   documents() -> 全部已打开文档（按 AcApDocumentIterator 顺序）
    //   documentAt(i)-> 第 i 个文档
    //   saveAll()    -> 逐个同步保存，返回 ["<路径> -> eOk", ...]
    //   quit()       -> 给 MDI 活动文档发 _.QUIT（脏文档会弹保存提示，先 saveAll）
    std::vector<PyApDocument> documents() const;
    PyApDocument documentAt(int index) const;
    std::vector<std::wstring> saveAll();
    std::wstring quit();

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
