//-----------------------------------------------------------------------------
// PyApDocManager.cpp — PyArxCore 的 DocumentManager 包装，对齐 .NET
//   Autodesk.AutoCAD.ApplicationServices.DocumentManager。
//
// 约定（PyWxArx 规范）：SDK 调用失败不抛异常；返回 ErrorStatus 字符串；
// 查询失败返回默认值/空文档。
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------

#include "PyApDocManager.h"
#include "PyApDocument.h"
#include <acdocman.h>

namespace
{
    const wchar_t* ErrName(Acad::ErrorStatus es)
    {
        switch (es) {
        case Acad::eOk: return L"eOk";
        case Acad::eNullObjectPointer: return L"eNullObjectPointer";
        case Acad::eInvalidInput: return L"eInvalidInput";
        case Acad::eLockViolation: return L"eLockViolation";
        case Acad::eNoDocument: return L"eNoDocument";
        case Acad::eWrongDatabase: return L"eWrongDatabase";
        case Acad::eFileNotFound: return L"eFileNotFound";
        default: return L"eUnknown";
        }
    }
}

int PyApDocManager::count() const {
    return impObj() ? impObj()->documentCount() : 0;
}

bool PyApDocManager::isApplicationContext() const {
    return impObj() && impObj()->isApplicationContext();
}

PyApDocument PyApDocManager::mdiActiveDocument() const {
    return impObj() ? PyApDocument(impObj()->mdiActiveDocument()) : PyApDocument(nullptr);
}

PyApDocument PyApDocManager::curDocument() const {
    return impObj() ? PyApDocument(impObj()->curDocument()) : PyApDocument(nullptr);
}

PyApDocument PyApDocManager::getDocument(const PyDbDatabase& db) const {
    if (!impObj() || !db.impObj()) return PyApDocument(nullptr);
    return PyApDocument(impObj()->document(db.impObj()));
}

std::wstring PyApDocManager::lockDocument(PyApDocument doc) {
    if (!impObj() || !doc.impObj()) return L"eNullObjectPointer";
    return ErrName(impObj()->lockDocument(doc.impObj(), AcAp::kWrite));
}

std::wstring PyApDocManager::unlockDocument(PyApDocument doc) {
    if (!impObj() || !doc.impObj()) return L"eNullObjectPointer";
    return ErrName(impObj()->unlockDocument(doc.impObj()));
}

std::wstring PyApDocManager::setCurrentDocument(PyApDocument doc, bool activate) {
    if (!impObj() || !doc.impObj()) return L"eNullObjectPointer";
    return ErrName(impObj()->setCurDocument(doc.impObj(),
                        AcAp::kNone, activate));
}

std::wstring PyApDocManager::activateDocument(PyApDocument doc) {
    if (!impObj() || !doc.impObj()) return L"eNullObjectPointer";
    return ErrName(impObj()->activateDocument(doc.impObj(), false));
}

std::wstring PyApDocManager::sendStringToExecute(PyApDocument doc,
                                                 const std::wstring& command,
                                                 bool activate,
                                                 bool wrapUpInactiveDoc,
                                                 bool echoCommand) {
    if (!impObj() || !doc.impObj()) return L"eNullObjectPointer";
    return ErrName(impObj()->sendStringToExecute(doc.impObj(), command.c_str(),
                        activate, wrapUpInactiveDoc, echoCommand));
}

std::wstring PyApDocManager::openDocument(const std::wstring& path) {
    if (!impObj()) return L"eNullObjectPointer";
    return ErrName(impObj()->appContextOpenDocument(path.c_str()));
}

std::wstring PyApDocManager::newDocument(const std::wstring& templateName) {
    if (!impObj()) return L"eNullObjectPointer";
    return ErrName(impObj()->appContextNewDocument(templateName.c_str()));
}

AcString PyApDocManager::toString() const {
    return AcString(std::format(L"PyApDocManager[documents={}]", count()).c_str());
}

void bind_PyApDocManager(pym& m) {
    py::class_<PyApDocManager>(m, "DocumentManager")
        .def(py::init<AcApDocManager*>(), py::arg("object"))
        // ---- camelCase（C++ 风格，主接口）----
        .def_property_readonly("count", &PyApDocManager::count, "文档数量")
        .def_property_readonly("isApplicationContext", &PyApDocManager::isApplicationContext,
            "是否处于 Application 上下文")
        .def_property_readonly("mdiActiveDocument", &PyApDocManager::mdiActiveDocument,
            "MDI 活动文档（PyArxCore.Ap.Document）")
        .def_property_readonly("curDocument", &PyApDocManager::curDocument,
            "当前上下文文档（PyArxCore.Ap.Document）")
        .def("getDocument", &PyApDocManager::getDocument, py::arg("database"),
            "按数据库对象取关联文档")
        .def("lockDocument", &PyApDocManager::lockDocument, py::arg("document"),
            "锁定文档，返回 ErrorStatus 字符串")
        .def("unlockDocument", &PyApDocManager::unlockDocument, py::arg("document"),
            "解锁文档，返回 ErrorStatus 字符串")
        .def("setCurrentDocument", &PyApDocManager::setCurrentDocument,
            py::arg("document"), py::arg("activate") = true,
            "将文档设为当前上下文文档")
        .def("activateDocument", &PyApDocManager::activateDocument, py::arg("document"),
            "激活文档")
        .def("sendStringToExecute", &PyApDocManager::sendStringToExecute,
            py::arg("document"), py::arg("command"),
            py::arg("activate") = true, py::arg("wrap_up_inactive_doc") = false,
            py::arg("echo_command") = true, "向文档发送命令字符串")
        .def("openDocument", &PyApDocManager::openDocument, py::arg("path"),
            "在应用上下文打开文档，返回 ErrorStatus 字符串")
        .def("newDocument", &PyApDocManager::newDocument, py::arg("template_name"),
            "在应用上下文新建文档，返回 ErrorStatus 字符串")
        .def("__repr__", &PyApDocManager::toString);
}
