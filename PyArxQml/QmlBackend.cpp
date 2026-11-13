// QmlBackend.cpp —— QML ↔ C++ ↔ Python 通路。
#include <Python.h>      // 必须先于 Qt 头文件，编译时还要开 QT_NO_KEYWORDS
#include "QmlBackend.h"
#include "PyArxQmlLog.h"
#include <QtCore/QElapsedTimer>
#include <QtCore/QByteArray>

// PyArxQmlModule.cpp 提供：当前注册的 Python 回调（借用引用）
PyObject* PyArxQmlCallback();

static QString CallPython(const char* op, int a, int b)
{
    PyObject* fn = PyArxQmlCallback();
    if (!fn) return QStringLiteral("<未注册 Python 回调>");

    const PyGILState_STATE gil = PyGILState_Ensure();
    QElapsedTimer t;
    t.start();

    QString out;
    PyObject* r = PyObject_CallFunction(fn, "sii", op, a, b);
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

    LogWrite("backend.%s(a=%d, b=%d) -> %s，往返 %.2f ms",
             op, a, b, out.toUtf8().constData(), ms);
    return out;
}

QmlBackend::QmlBackend(QObject* parent) : QObject(parent) {}

void QmlBackend::addNumbers(int a, int b)
{
    setResult(CallPython("add", a, b));
}

void QmlBackend::reportClick(const QString& what)
{
    LogWrite("QML 回报点击: %s", what.toUtf8().constData());
    setStatus(QStringLiteral("QML 已把点击回报给 C++：") + what);
}

QString QmlBackend::call(const QString& method)
{
    return CallPython(method.toUtf8().constData(), 0, 0);
}

void QmlBackend::setResult(const QString& r)
{
    if (m_result == r) return;
    m_result = r;
    Q_EMIT resultChanged();
}

void QmlBackend::setStatus(const QString& s)
{
    if (m_status == s) return;
    m_status = s;
    Q_EMIT statusChanged();
}
