#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDatabaseReactor.h"
#include "PyDbDatabase.h"
#include "PyDbObject.h"

// 所有权说明（待 H1/H6 统一处理，本批 H3 不改动）：回调内 PyDbDatabase(pDb)/PyDbObject(pObj)
// 为临时包装，对象生命周期由 AutoCAD 回调方负责，本包装不应 close/delete。

void PyDbDatabaseReactor::objectAppended(const AcDbDatabase* pDb, const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "objectAppended")) {
        try { py_self.attr("objectAppended")(PyDbDatabase(const_cast<AcDbDatabase*>(pDb)), PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (const pybind11::error_already_set& e) { acutPrintf(L"\n[PyDbDatabaseReactor] Python 回调异常: %hs\n", e.what()); PyErr_Clear(); }
        catch (...) { acutPrintf(L"\n[PyDbDatabaseReactor] 回调抛出未知异常（已忽略）\n"); }
    } else {
        AcDbDatabaseReactor::objectAppended(pDb, pObj);
    }
}

void PyDbDatabaseReactor::objectUnAppended(const AcDbDatabase* pDb, const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "objectUnAppended")) {
        try { py_self.attr("objectUnAppended")(PyDbDatabase(const_cast<AcDbDatabase*>(pDb)), PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (const pybind11::error_already_set& e) { acutPrintf(L"\n[PyDbDatabaseReactor] Python 回调异常: %hs\n", e.what()); PyErr_Clear(); }
        catch (...) { acutPrintf(L"\n[PyDbDatabaseReactor] 回调抛出未知异常（已忽略）\n"); }
    } else {
        AcDbDatabaseReactor::objectUnAppended(pDb, pObj);
    }
}

void PyDbDatabaseReactor::objectReAppended(const AcDbDatabase* pDb, const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "objectReAppended")) {
        try { py_self.attr("objectReAppended")(PyDbDatabase(const_cast<AcDbDatabase*>(pDb)), PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (const pybind11::error_already_set& e) { acutPrintf(L"\n[PyDbDatabaseReactor] Python 回调异常: %hs\n", e.what()); PyErr_Clear(); }
        catch (...) { acutPrintf(L"\n[PyDbDatabaseReactor] 回调抛出未知异常（已忽略）\n"); }
    } else {
        AcDbDatabaseReactor::objectReAppended(pDb, pObj);
    }
}

void PyDbDatabaseReactor::objectOpenedForModify(const AcDbDatabase* pDb, const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "objectOpenedForModify")) {
        try { py_self.attr("objectOpenedForModify")(PyDbDatabase(const_cast<AcDbDatabase*>(pDb)), PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (const pybind11::error_already_set& e) { acutPrintf(L"\n[PyDbDatabaseReactor] Python 回调异常: %hs\n", e.what()); PyErr_Clear(); }
        catch (...) { acutPrintf(L"\n[PyDbDatabaseReactor] 回调抛出未知异常（已忽略）\n"); }
    } else {
        AcDbDatabaseReactor::objectOpenedForModify(pDb, pObj);
    }
}

void PyDbDatabaseReactor::objectModified(const AcDbDatabase* pDb, const AcDbObject* pObj) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "objectModified")) {
        try { py_self.attr("objectModified")(PyDbDatabase(const_cast<AcDbDatabase*>(pDb)), PyDbObject(const_cast<AcDbObject*>(pObj))); }
        catch (const pybind11::error_already_set& e) { acutPrintf(L"\n[PyDbDatabaseReactor] Python 回调异常: %hs\n", e.what()); PyErr_Clear(); }
        catch (...) { acutPrintf(L"\n[PyDbDatabaseReactor] 回调抛出未知异常（已忽略）\n"); }
    } else {
        AcDbDatabaseReactor::objectModified(pDb, pObj);
    }
}

void PyDbDatabaseReactor::objectErased(const AcDbDatabase* pDb, const AcDbObject* pObj, bool erased) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "objectErased")) {
        try { py_self.attr("objectErased")(PyDbDatabase(const_cast<AcDbDatabase*>(pDb)), PyDbObject(const_cast<AcDbObject*>(pObj)), erased); }
        catch (const pybind11::error_already_set& e) { acutPrintf(L"\n[PyDbDatabaseReactor] Python 回调异常: %hs\n", e.what()); PyErr_Clear(); }
        catch (...) { acutPrintf(L"\n[PyDbDatabaseReactor] 回调抛出未知异常（已忽略）\n"); }
    } else {
        AcDbDatabaseReactor::objectErased(pDb, pObj, erased);
    }
}

void PyDbDatabaseReactor::headerSysVarWillChange(const AcDbDatabase* pDb, const wchar_t* name) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "headerSysVarWillChange")) {
        try { py_self.attr("headerSysVarWillChange")(PyDbDatabase(const_cast<AcDbDatabase*>(pDb)), pybind11::str(AcString(name).utf8Ptr())); }
        catch (const pybind11::error_already_set& e) { acutPrintf(L"\n[PyDbDatabaseReactor] Python 回调异常: %hs\n", e.what()); PyErr_Clear(); }
        catch (...) { acutPrintf(L"\n[PyDbDatabaseReactor] 回调抛出未知异常（已忽略）\n"); }
    } else {
        AcDbDatabaseReactor::headerSysVarWillChange(pDb, name);
    }
}

void PyDbDatabaseReactor::headerSysVarChanged(const AcDbDatabase* pDb, const wchar_t* name, bool success) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "headerSysVarChanged")) {
        try { py_self.attr("headerSysVarChanged")(PyDbDatabase(const_cast<AcDbDatabase*>(pDb)), pybind11::str(AcString(name).utf8Ptr()), success); }
        catch (const pybind11::error_already_set& e) { acutPrintf(L"\n[PyDbDatabaseReactor] Python 回调异常: %hs\n", e.what()); PyErr_Clear(); }
        catch (...) { acutPrintf(L"\n[PyDbDatabaseReactor] 回调抛出未知异常（已忽略）\n"); }
    } else {
        AcDbDatabaseReactor::headerSysVarChanged(pDb, name, success);
    }
}

void PyDbDatabaseReactor::goodbye(const AcDbDatabase* pDb) {
    pybind11::gil_scoped_acquire gil;
    pybind11::object py_self = pybind11::none();
    bool h3_bound = true;
    try {
        py_self = pybind11::cast(this, pybind11::return_value_policy::reference);
    } catch (const pybind11::cast_error&) { h3_bound = false; }
    if (h3_bound && pybind11::hasattr(py_self, "goodbye")) {
        try { py_self.attr("goodbye")(PyDbDatabase(const_cast<AcDbDatabase*>(pDb))); }
        catch (const pybind11::error_already_set& e) { acutPrintf(L"\n[PyDbDatabaseReactor] Python 回调异常: %hs\n", e.what()); PyErr_Clear(); }
        catch (...) { acutPrintf(L"\n[PyDbDatabaseReactor] 回调抛出未知异常（已忽略）\n"); }
    } else {
        AcDbDatabaseReactor::goodbye(pDb);
    }
}

bool PyDbDatabaseReactor::attach(PyDbDatabase& db) {
	if (db.isNull()) return false;
	return PyWarnIfError(db.impObj()->addReactor(this), L"DatabaseReactor.attach");
}

bool PyDbDatabaseReactor::detach(PyDbDatabase& db) {
	if (db.isNull()) return false;
	return PyWarnIfError(db.impObj()->removeReactor(this), L"DatabaseReactor.detach");
}

void bind_PyDbDatabaseReactor(pym& m) {
	py::class_<PyDbDatabaseReactor>(m, "DatabaseReactor",
        "数据库反应器基类。Python 子类继承并重写方法以接收数据库事件回调。")
		.def(py::init<>())
        .def("objectAppended", &PyDbDatabaseReactor::objectAppended, "对象追加到数据库时回调")
        .def("objectUnAppended", &PyDbDatabaseReactor::objectUnAppended, "对象从数据库移除时回调")
        .def("objectReAppended", &PyDbDatabaseReactor::objectReAppended, "对象重新追加时回调")
        .def("objectOpenedForModify", &PyDbDatabaseReactor::objectOpenedForModify, "对象打开用于修改时回调")
        .def("objectModified", &PyDbDatabaseReactor::objectModified, "对象被修改后回调")
        .def("objectErased", &PyDbDatabaseReactor::objectErased, "对象被删除/恢复时回调")
        .def("headerSysVarWillChange", &PyDbDatabaseReactor::headerSysVarWillChange, "系统变量即将修改时回调")
        .def("headerSysVarChanged", &PyDbDatabaseReactor::headerSysVarChanged, "系统变量修改完成时回调")
        .def("goodbye", &PyDbDatabaseReactor::goodbye, "数据库即将销毁时回调")
		.def("attach", &PyDbDatabaseReactor::attach, py::arg("db"))
		.def("detach", &PyDbDatabaseReactor::detach, py::arg("db"));
}

