#pragma once
//-----------------------------------------------------------------------------
// PyApDocument.h — PyArxCore 的 Document 包装，对齐 .NET
//   Autodesk.AutoCAD.ApplicationServices.Document [Wrapper("AcApDocument")]。
//
// 对齐成员（C# -> ObjectARX）：
//   Name              -> fileName()（完整路径名）
//   Database          -> database()（返回 PyDbDatabase 包装）
//   IsActive          -> 当前 == acDocManager->mdiActiveDocument()
//   IsNamedDrawing    -> isNamedDrawing()
//   IsReadOnly        -> isReadOnly()
//   IsQuiescent       -> isQuiescent()（无命令/Lisp/脚本/ARX 活动）
//   SendStringToExecute -> acDocManager->sendStringToExecute(pDoc, ...)
//   LockDocument      -> acDocManager->lockDocument()
//   UnlockDocument    -> acDocManager->unlockDocument()
//   LockMode          -> lockMode()
//   PushDbmod/PopDbmod-> pushDbmod()/popDbmod()
//   UpgradeDocOpen/DowngradeDocOpen -> upgradeDocOpen()/downgradeDocOpen()
//   TransactionManager-> transactionManager()（指针，暂不展开）
//   DocTitle          -> docTitle()/setDocTitle()
//   Window            -> 关联文档窗口 HWND（经 AcApDocWindow 查询，暂返回 cDoc/句柄）
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <acdocman.h>
#include "PyDbDatabase.h"

class PyApDocument {
    AcApDocument* m_ptr = nullptr;
public:
    explicit PyApDocument(AcApDocument* p = nullptr) : m_ptr(p) {}
    AcApDocument* impObj() const { return m_ptr; }

    // C# Document 对齐成员
    std::wstring name() const;
    PyDbDatabase database() const;
    bool isActive() const;
    bool isNamedDrawing() const;
    bool isReadOnly() const;
    bool isQuiescent() const;
    std::wstring docTitle() const;
    void setDocTitle(const std::wstring& title);

    // 文档操作（返回 ErrorStatus 字符串，按 PyWxArx 约定不抛异常）
    std::wstring sendStringToExecute(const std::wstring& command, bool activate,
                                     bool wrapUpInactiveDoc, bool echoCommand);
    std::wstring lockDocument();
    std::wstring unlockDocument();
    int lockMode(bool includeMyLocks) const;
    std::wstring pushDbmod();
    std::wstring popDbmod();
    std::wstring upgradeDocOpen();
    std::wstring downgradeDocOpen(bool promptForSave);
    bool setFocus();          // 对齐 C#：文档窗口获得输入焦点
    std::wstring activate();  // 激活文档（acDocManager->activateDocument）

    long long windowHandle() const;
    AcString toString() const;
};

void bind_PyApDocument(pym& m);
