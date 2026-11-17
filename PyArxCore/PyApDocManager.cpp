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

std::vector<PyApDocument> PyApDocManager::documents() const {
    std::vector<PyApDocument> out;
    AcApDocManager* mgr = impObj() ? impObj() : ::acDocManager;
    if (!mgr) return out;
    auto it = mgr->getDocumentIterator();
    if (!it) return out;
    for (; !it->done(); it->step()) {
        AcApDocument* d = it->document();
        if (d) out.emplace_back(d);
    }
    return out;
}

PyApDocument PyApDocManager::documentAt(int index) const {
    if (index < 0) return PyApDocument(nullptr);
    auto docs = documents();
    if (static_cast<std::size_t>(index) >= docs.size()) return PyApDocument(nullptr);
    return docs[static_cast<std::size_t>(index)];
}

std::vector<std::wstring> PyApDocManager::saveAll() {
    std::vector<std::wstring> out;
    for (auto& d : documents()) {
        std::wstring nm = d.name();
        std::wstring st = d.save();
        out.push_back(nm + L" -> " + st);
    }
    return out;
}

std::wstring PyApDocManager::quit() {
    AcApDocManager* mgr = impObj() ? impObj() : ::acDocManager;
    if (!mgr) return L"eNullObjectPointer";
    AcApDocument* d = mgr->mdiActiveDocument();
    if (!d) return L"eNoDocument";
    return ErrName(mgr->sendStringToExecute(d, L"_.QUIT ", true, false, false));
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
        .def_property_readonly("documents", &PyApDocManager::documents,
            "全部已打开文档（list[Ap.Document]）")
        .def("documentAt", &PyApDocManager::documentAt, py::arg("index"),
            "按序号取文档，越界返回空文档")
        .def("saveAll", &PyApDocManager::saveAll,
            "逐个同步保存所有文档，返回 [\"路径 -> ErrorStatus\"]")
        .def("quit", &PyApDocManager::quit,
            "向 MDI 活动文档发送 _.QUIT 关闭 AutoCAD（有脏文档会弹保存提示，先 saveAll）")
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
