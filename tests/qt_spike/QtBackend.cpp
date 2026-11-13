// QtBackend.cpp —— QML ↔ C++ ↔ Python 的双向通路实现。
#include <Python.h>          // 必须先于 Qt 头文件，且编译时开 QT_NO_KEYWORDS
#include "QtBackend.h"
#include <QtCore/QElapsedTimer>
#include <QtCore/QDebug>
#include <cstdio>

// QtQuickSpike.cpp 里注册的 Python 回调（借用引用）
PyObject* qt_backend_callback();
void qt_backend_log(const char* msg);

static QString callPy(PyObject* fn, int a, int b)
{
    if (!fn) return QStringLiteral("<未注册 Python 回调>");

    const PyGILState_STATE gil = PyGILState_Ensure();
    QElapsedTimer t;
    t.start();

    QString out;
    PyObject* r = PyObject_CallFunction(fn, "ii", a, b);
    if (!r) {
        PyErr_Print();
        out = QStringLiteral("<Python 抛异常>");
    } else {
        PyObject* s = PyObject_Str(r);
        out = QString::fromUtf8(PyUnicode_AsUTF8(s));
        Py_DECREF(s);
        Py_DECREF(r);
    }
    const double ms = (double)t.nsecsElapsed() / 1e6;

    PyGILState_Release(gil);

    char buf[128];
    std::snprintf(buf, sizeof(buf), "addNumbers(%d, %d) -> %s，往返 %.2f ms",
                  a, b, out.toUtf8().constData(), ms);
    qt_backend_log(buf);
    return out;
}

QtBackend::QtBackend(QObject* parent) : QObject(parent) {}

void QtBackend::addNumbers(int a, int b)
{
    setResult(callPy(qt_backend_callback(), a, b));
}

void QtBackend::reportClick(const QString& what)
{
    qt_backend_log(("QML 回报点击: " + what).toUtf8().constData());
    setStatus(QStringLiteral("QML 已把点击回报给 C++：") + what);
}

void QtBackend::setResult(const QString& r)
{
    if (m_result == r) return;
    m_result = r;
    Q_EMIT resultChanged();
}

void QtBackend::setStatus(const QString& s)
{
    if (m_status == s) return;
    m_status = s;
    Q_EMIT statusChanged();
}
