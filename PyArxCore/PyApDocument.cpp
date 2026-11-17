//-----------------------------------------------------------------------------
// PyApDocument.cpp — PyArxCore 的 Document 包装，对齐 .NET
//   Autodesk.AutoCAD.ApplicationServices.Document。
//
// 约定（PyWxArx 规范）：SDK 调用失败不抛异常、不崩溃；返回 ErrorStatus 字符串
// （"eOk" 表示成功）；查询失败返回默认值。
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------

#include "PyApDocument.h"
#include <acdocman.h>
#include <AcApDocWindow.h>
#include <algorithm>
#include <cwctype>

namespace
{
    // Acad::ErrorStatus -> 稳定字符串（供 Python 侧判断）
    const wchar_t* ErrName(Acad::ErrorStatus es)
    {
        switch (es) {
        case Acad::eOk: return L"eOk";
        case Acad::eNullObjectPointer: return L"eNullObjectPointer";
        case Acad::eInvalidInput: return L"eInvalidInput";
        case Acad::eLockViolation: return L"eLockViolation";
        case Acad::eNotApplicable: return L"eNotApplicable";
        case Acad::eWrongDatabase: return L"eWrongDatabase";
        case Acad::eNoDocument: return L"eNoDocument";
        default: return L"eUnknown";
        }
    }

    AcApDocManager* DocMgr()
    {
        return ::acDocManager;
    }
}

std::wstring PyApDocument::name() const
{
    if (!m_ptr) return L"";
    const ACHAR* f = m_ptr->fileName();
    return f ? std::wstring(f) : L"";
}

PyDbDatabase PyApDocument::database() const
{
    if (!m_ptr) return PyDbDatabase(nullptr);
    return PyDbDatabase(m_ptr->database(), false);
}

bool PyApDocument::isActive() const
{
    if (!m_ptr || !DocMgr()) return false;
    return DocMgr()->mdiActiveDocument() == m_ptr;
}

bool PyApDocument::isNamedDrawing() const
{
    return m_ptr ? m_ptr->isNamedDrawing() : false;
}

bool PyApDocument::isReadOnly() const
{
    return m_ptr ? m_ptr->isReadOnly() : false;
}

bool PyApDocument::isQuiescent() const
{
    return m_ptr ? m_ptr->isQuiescent() : false;
}

std::wstring PyApDocument::docTitle() const
{
    if (!m_ptr) return L"";
    const ACHAR* t = m_ptr->docTitle();
    return t ? std::wstring(t) : L"";
}

void PyApDocument::setDocTitle(const std::wstring& title)
{
    if (m_ptr) m_ptr->setDocTitle(title.c_str());
}

std::wstring PyApDocument::sendStringToExecute(const std::wstring& command,
                                               bool activate,
                                               bool wrapUpInactiveDoc,
                                               bool echoCommand)
{
    if (!m_ptr || !DocMgr())
        return L"eNullObjectPointer";
    Acad::ErrorStatus es = DocMgr()->sendStringToExecute(
        m_ptr, command.c_str(), activate, wrapUpInactiveDoc, echoCommand);
    return ErrName(es);
}

std::wstring PyApDocument::lockDocument()
{
    if (!m_ptr || !DocMgr())
        return L"eNullObjectPointer";
    Acad::ErrorStatus es = DocMgr()->lockDocument(m_ptr, AcAp::kWrite);
    return ErrName(es);
}

std::wstring PyApDocument::unlockDocument()
{
    if (!m_ptr || !DocMgr())
        return L"eNullObjectPointer";
    Acad::ErrorStatus es = DocMgr()->unlockDocument(m_ptr);
    return ErrName(es);
}

int PyApDocument::lockMode(bool includeMyLocks) const
{
    if (!m_ptr) return -1;
    return static_cast<int>(m_ptr->lockMode(includeMyLocks));
}

std::wstring PyApDocument::pushDbmod()
{
    if (!m_ptr) return L"eNullObjectPointer";
    m_ptr->pushDbmod();
    return L"eOk";
}

std::wstring PyApDocument::popDbmod()
{
    if (!m_ptr) return L"eNullObjectPointer";
    return ErrName(m_ptr->popDbmod());
}

std::wstring PyApDocument::upgradeDocOpen()
{
    if (!m_ptr) return L"eNullObjectPointer";
    return ErrName(m_ptr->upgradeDocOpen());
}

std::wstring PyApDocument::downgradeDocOpen(bool promptForSave)
{
    if (!m_ptr) return L"eNullObjectPointer";
    return ErrName(m_ptr->downgradeDocOpen(promptForSave));
}

long long PyApDocument::windowHandle() const
{
    if (!m_ptr) return 0;
    AcApDocWindowManager* wm = ::acDocWindowManager;
    if (!wm) return 0;
    // 遍历文档窗口，匹配本文档对应的 DWG 子窗口
    std::unique_ptr<AcApDocWindowIterator> it(wm->newIterator());
    if (!it) return 0;
    for (; !it->done(); it->step()) {
        AcApDocWindow* w = it->current();
        if (!w) continue;
        AcApDwgDocWindow* dwg = dynamic_cast<AcApDwgDocWindow*>(w);
        if (dwg && dwg->document() == m_ptr) {
            HWND h = w->windowHandle();
            return static_cast<long long>(reinterpret_cast<std::intptr_t>(h));
        }
    }
    return 0;
}

bool PyApDocument::setFocus()
{
    if (!m_ptr) return false;
    long long h = windowHandle();
    if (!h) return false;
    HWND hwnd = reinterpret_cast<HWND>(static_cast<std::intptr_t>(h));
    ::ShowWindow(hwnd, SW_RESTORE);
    ::SetForegroundWindow(hwnd);
    ::SetFocus(hwnd);
    return true;
}

std::wstring PyApDocument::activate()
{
    if (!m_ptr || !DocMgr()) return L"eNullObjectPointer";
    return ErrName(DocMgr()->activateDocument(m_ptr, false));
}

std::wstring PyApDocument::save()
{
    if (!m_ptr || !DocMgr()) return L"eNullObjectPointer";
    const ACHAR* f = m_ptr->fileName();
    if (!f || !*f) return L"eInvalidInput";   // 未命名图形没有文件名

    // 文档库不能直接 db->save()（实测 eFileInternalErr=74）；
    // 但 db->saveAs(当前文件名) 是同步落盘、不弹任何对话框的，实测 eOk。
    // DXF 图形走 dxfOut，避免把 DWG 字节写进 .dxf。
    AcDbDatabase* db = m_ptr->database();
    if (!db) return L"eNullObjectPointer";

    std::wstring path(f);
    const std::wstring low = [&] {
        std::wstring s = path;
        std::transform(s.begin(), s.end(), s.begin(), ::towlower);
        return s;
    }();
    Acad::ErrorStatus es;
    if (low.size() >= 4 && low.compare(low.size() - 4, 4, L".dxf") == 0)
        es = db->dxfOut(path.c_str());
    else
        // 必须用默认的 bBakAndRename=true：传 false 对文档库返回非 eOk（实测 eUnknown/访问错误）；
        // 默认档实测同步落盘且无任何对话框。
        es = db->saveAs(path.c_str());

    return ErrName(es);
}

std::wstring PyApDocument::saveAs(const std::wstring& path)
{
    if (!m_ptr || !DocMgr()) return L"eNullObjectPointer";
    // SAVEAS 先问"文件格式"，给空串会一直停在提示上（实测会卡死命令上下文）；
    // 必须显式给格式，再给路径，结尾留空格表示回车。
    std::wstring cmd = L"_.SAVEAS 2018 \"" + path + L"\" ";
    return ErrName(DocMgr()->sendStringToExecute(m_ptr, cmd.c_str(), false, true, false));
}

AcString PyApDocument::toString() const
{
    if (!m_ptr)
        return AcString(L"PyApDocument[null]");
    return AcString(std::format(L"PyApDocument[{}]", name()).c_str());
}

// ===========================================================================
// 绑定
// ===========================================================================
void bind_PyApDocument(pym& m)
{
    py::class_<PyApDocument>(m, "Document")
        .def(py::init<>())
        // ---- camelCase（C++ 风格，主接口）----
        .def_property_readonly("name", &PyApDocument::name, "文档完整路径名")
        .def_property_readonly("database", &PyApDocument::database, "文档数据库（PyDbDatabase）")
        .def_property_readonly("isActive", &PyApDocument::isActive, "是否为 MDI 活动文档")
        .def_property_readonly("isNamedDrawing", &PyApDocument::isNamedDrawing, "是否已命名图形")
        .def_property_readonly("isReadOnly", &PyApDocument::isReadOnly, "是否只读")
        .def_property_readonly("isQuiescent", &PyApDocument::isQuiescent, "是否空闲（无命令/Lisp 活动）")
        .def_property_readonly("docTitle", &PyApDocument::docTitle, "文档窗口标题")
        .def_property_readonly("windowHandle", &PyApDocument::windowHandle, "文档窗口 HWND")
        .def("setDocTitle", &PyApDocument::setDocTitle, py::arg("title"))
        .def("sendStringToExecute", &PyApDocument::sendStringToExecute,
             py::arg("command"), py::arg("activate") = true,
             py::arg("wrap_up_inactive_doc") = false, py::arg("echo_command") = true,
             "发送命令字符串到文档执行，返回 ErrorStatus 字符串")
        .def("lockDocument", &PyApDocument::lockDocument, "锁定文档，返回 ErrorStatus 字符串")
        .def("unlockDocument", &PyApDocument::unlockDocument, "解锁文档，返回 ErrorStatus 字符串")
        .def("lockMode", &PyApDocument::lockMode, py::arg("include_my_locks") = false,
             "文档锁定模式（AcAp::DocLockMode 值）")
        .def("pushDbmod", &PyApDocument::pushDbmod)
        .def("popDbmod", &PyApDocument::popDbmod)
        .def("upgradeDocOpen", &PyApDocument::upgradeDocOpen)
        .def("downgradeDocOpen", &PyApDocument::downgradeDocOpen, py::arg("prompt_for_save") = true)
        .def("setFocus", &PyApDocument::setFocus, "文档窗口获得输入焦点")
        .def("activate", &PyApDocument::activate, "激活文档，返回 ErrorStatus 字符串")
        .def("save", &PyApDocument::save, "按当前文件名同步保存，返回 ErrorStatus 字符串")
        .def("saveAs", &PyApDocument::saveAs, py::arg("path"), "另存为，返回 ErrorStatus 字符串")
        .def("__repr__", &PyApDocument::toString);
}
