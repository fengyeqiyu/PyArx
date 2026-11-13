// PyArxQmlModule.cpp —— Python 模块 PyArxQml 的绑定层。
// 双向通道：
//   QML -> Python：set_callback(fn)，QML 里 backend.call("名字") / backend.addNumbers()
//   Python -> QML：set_prop / get_prop / set_named / get_named / call / call_named
#include <Python.h>          // 必须先于 Qt 头文件
#include "PyArxQml.h"
#include "PyArxQmlLog.h"

#include <QtCore/QVariant>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>
#include <QtCore/QString>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QElapsedTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QFontDatabase>
#include <QtGui/QFont>
#include <QtGui/QFontInfo>
#include <QtGui/QScreen>
#include <QtQml/QQmlProperty>
#include <QtQml/QQmlExpression>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtQml/QJSValue>
#include <QtQuick/QQuickItem>
#include <QtCore/QList>

#include <cstdio>
#include <cstring>
#include <string>

using pyarx::qml::QmlHost;
using pyarx::qml::HostStats;

static PyObject* g_cb = nullptr;

PyObject* PyArxQmlCallback() { return g_cb; }

// ---------- 值转换 ----------

static QVariant FromPy(PyObject* o)
{
    if (!o || o == Py_None) return QVariant();

    if (PyBool_Check(o)) return QVariant(o == Py_True);

    if (PyLong_Check(o)) {
        const long long v = PyLong_AsLongLong(o);
        if (v == -1 && PyErr_Occurred()) {   // 超出 long long（如 numpy int64 大值）
            PyErr_Clear();
            return QVariant(PyLong_AsDouble(o));
        }
        return QVariant(v);
    }

    if (PyFloat_Check(o)) return QVariant(PyFloat_AsDouble(o));

    if (PyUnicode_Check(o)) {
        PyObject* u = PyUnicode_AsUTF8String(o);
        if (!u) { PyErr_Clear(); return QVariant(); }
        const QString s = QString::fromUtf8(PyBytes_AsString(u));
        Py_DECREF(u);
        return QVariant(s);
    }

    if (PyList_Check(o) || PyTuple_Check(o)) {
        QVariantList list;
        PyObject* seq = PySequence_Fast(o, "需要序列");
        if (!seq) { PyErr_Clear(); return QVariant(); }
        const Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
        for (Py_ssize_t i = 0; i < n; ++i)
            list.append(FromPy(PySequence_Fast_GET_ITEM(seq, i)));
        Py_DECREF(seq);
        return QVariant(list);
    }

    if (PyDict_Check(o)) {
        QVariantMap map;
        PyObject *k = nullptr, *v = nullptr;
        Py_ssize_t pos = 0;
        while (PyDict_Next(o, &pos, &k, &v)) {
            PyObject* ks = PyObject_Str(k);
            if (!ks) { PyErr_Clear(); continue; }
            map.insert(QString::fromUtf8(PyUnicode_AsUTF8(ks)), FromPy(v));
            Py_DECREF(ks);
        }
        return QVariant(map);
    }

    PyObject* s = PyObject_Str(o);
    if (!s) { PyErr_Clear(); return QVariant(); }
    const QString str = QString::fromUtf8(PyUnicode_AsUTF8(s));
    Py_DECREF(s);
    return QVariant(str);
}

static PyObject* ToPy(const QVariant& v)
{
    if (!v.isValid()) Py_RETURN_NONE;

    // QQmlExpression 求值出来的数组/对象是 QJSValue，不先转会把 list 变成空字符串
    if (v.userType() == qMetaTypeId<QJSValue>()) {
        const QJSValue js = v.value<QJSValue>();
        if (js.isUndefined() || js.isNull()) Py_RETURN_NONE;
        if (js.isBool()) return PyBool_FromLong(js.toBool() ? 1 : 0);
        if (js.isNumber()) {
            const double d = js.toNumber();
            if (d == (double)(long long)d && d > -9.2e18 && d < 9.2e18)
                return PyLong_FromLongLong((long long)d);
            return PyFloat_FromDouble(d);
        }
        if (js.isString()) return PyUnicode_FromString(js.toString().toUtf8().constData());
        return ToPy(js.toVariant());
    }

    switch ((int)v.typeId()) {
    case QMetaType::Bool:   return PyBool_FromLong(v.toBool() ? 1 : 0);
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::UChar:
        return PyLong_FromLongLong(v.toLongLong());
    case QMetaType::Double:
    case QMetaType::Float:
        return PyFloat_FromDouble(v.toDouble());
    case QMetaType::QString:
        return PyUnicode_FromString(v.toString().toUtf8().constData());
    case QMetaType::QStringList: {
        const QStringList sl = v.toStringList();
        PyObject* l = PyList_New(sl.size());
        for (int i = 0; i < sl.size(); ++i)
            PyList_SET_ITEM(l, i, PyUnicode_FromString(sl.at(i).toUtf8().constData()));
        return l;
    }
    case QMetaType::QVariantList: {
        const QVariantList vl = v.toList();
        PyObject* l = PyList_New(vl.size());
        for (int i = 0; i < vl.size(); ++i)
            PyList_SET_ITEM(l, i, ToPy(vl.at(i)));
        return l;
    }
    case QMetaType::QVariantMap: {
        const QVariantMap vm = v.toMap();
        PyObject* d = PyDict_New();
        for (auto it = vm.constBegin(); it != vm.constEnd(); ++it)
            PyDict_SetItemString(d, it.key().toUtf8().constData(), ToPy(it.value()));
        return d;
    }
    default:
        break;
    }

    // 其他类型（QColor / QPointF / QUrl ...）统一转字符串
    return PyUnicode_FromString(v.toString().toUtf8().constData());
}

// ---------- 属性 / 方法 ----------

static bool WriteProp(QObject* obj, const QString& name, const QVariant& v)
{
    QQmlProperty p(obj, name);
    if (p.isValid()) return p.write(v);
    return obj->setProperty(name.toUtf8().constData(), v);
}

static QVariant ReadProp(QObject* obj, const QString& name)
{
    QQmlProperty p(obj, name);
    if (p.isValid()) return p.read();
    return obj->property(name.toUtf8().constData());
}

static QVariant InvokeQml(QObject* obj, const QByteArray& method, const QVariantList& args)
{
    QVariant ret;
    bool ok = false;
    switch (args.size()) {
    case 0: ok = QMetaObject::invokeMethod(obj, method.constData(), Qt::DirectConnection,
                                          Q_RETURN_ARG(QVariant, ret)); break;
    case 1: ok = QMetaObject::invokeMethod(obj, method.constData(), Qt::DirectConnection,
                                          Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, args.at(0))); break;
    case 2: ok = QMetaObject::invokeMethod(obj, method.constData(), Qt::DirectConnection,
                                          Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, args.at(0)),
                                          Q_ARG(QVariant, args.at(1))); break;
    case 3: ok = QMetaObject::invokeMethod(obj, method.constData(), Qt::DirectConnection,
                                          Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, args.at(0)),
                                          Q_ARG(QVariant, args.at(1)), Q_ARG(QVariant, args.at(2))); break;
    case 4: ok = QMetaObject::invokeMethod(obj, method.constData(), Qt::DirectConnection,
                                          Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, args.at(0)),
                                          Q_ARG(QVariant, args.at(1)), Q_ARG(QVariant, args.at(2)),
                                          Q_ARG(QVariant, args.at(3))); break;
    default:
        LogWrite("call(%s) 参数超过 4 个，已忽略", method.constData());
        return QVariant();
    }
    if (!ok) {
        LogWrite("call(%s) 未找到该 QML 方法", method.constData());
        return QVariant();
    }
    return ret;
}

static QVariantList TupleToArgs(PyObject* args, Py_ssize_t from)
{
    QVariantList list;
    for (Py_ssize_t i = from; i < PyTuple_GET_SIZE(args); ++i)
        list.append(FromPy(PyTuple_GET_ITEM(args, i)));
    return list;
}

static QObject* Target(const std::string& objectName, bool* found)
{
    QObject* o = (QObject*)QmlHost::Instance().FindItem(objectName);
    if (found) *found = (o != nullptr);
    return o;
}

// ---------- QML 对象树枚举（不用手写 objectName 也能定位控件）---------

static QVariant PropIfAny(QObject* o, const char* name)
{
    const int idx = o->metaObject()->indexOfProperty(name);
    if (idx < 0) return QVariant();
    return o->metaObject()->property(idx).read(o);
}

static QString FriendlyClass(QObject* o)
{
    QString cls = QString::fromLatin1(o->metaObject()->className());
    cls.replace(QLatin1String("QQuick"), QString());
    if (cls == QLatin1String("Item")) return cls;
    return cls;
}

static void ChildList(QObject* o, QList<QObject*>& out)
{
    if (QQuickItem* it = qobject_cast<QQuickItem*>(o)) {
        const QList<QQuickItem*> kids = it->childItems();
        for (QQuickItem* k : kids) out.append(k);
        return;
    }
    out = o->children();
}

static void DumpNode(QObject* o, const QString& path, int depth, int maxDepth,
                     QVariantList& out, int& budget)
{
    if (!o || budget <= 0) return;
    --budget;

    QVariantMap m;
    m.insert(QStringLiteral("path"), path);
    m.insert(QStringLiteral("class"), QString::fromLatin1(o->metaObject()->className()));
    m.insert(QStringLiteral("type"), FriendlyClass(o));
    m.insert(QStringLiteral("objectName"), o->objectName());

    const QVariant text = PropIfAny(o, "text");
    if (text.isValid() && !text.toString().isEmpty())
        m.insert(QStringLiteral("text"), text.toString());

    if (QQuickItem* it = qobject_cast<QQuickItem*>(o)) {
        m.insert(QStringLiteral("x"), it->x());
        m.insert(QStringLiteral("y"), it->y());
        m.insert(QStringLiteral("w"), it->width());
        m.insert(QStringLiteral("h"), it->height());
        m.insert(QStringLiteral("visible"), it->isVisible());
    } else {
        const QVariant vis = PropIfAny(o, "visible");
        if (vis.isValid()) m.insert(QStringLiteral("visible"), vis.toBool());
    }
    out.append(m);

    if (depth >= maxDepth) return;
    QList<QObject*> kids;
    ChildList(o, kids);
    for (int i = 0; i < kids.size(); ++i) {
        const QString childPath = path.isEmpty() ? QString::number(i)
                                                 : path + QLatin1Char('/') + QString::number(i);
        DumpNode(kids.at(i), childPath, depth + 1, maxDepth, out, budget);
    }
}

static QObject* ResolvePath(const QString& path)
{
    QObject* root = (QObject*)QmlHost::Instance().RootObject();
    if (!root) return nullptr;
    if (path.isEmpty()) return root;

    QObject* cur = root;
    QStringList segs = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    // 根节点自己记作 "0"，解析时跳过这一段
    if (!segs.isEmpty() && segs.first() == QLatin1String("0")) segs.removeFirst();
    for (const QString& s : segs) {
        bool ok = false;
        const int idx = s.toInt(&ok);
        if (!ok) return nullptr;
        QList<QObject*> kids;
        ChildList(cur, kids);
        if (idx < 0 || idx >= kids.size()) return nullptr;
        cur = kids.at(idx);
    }
    return cur;
}


static PyObject* py_set_log(PyObject*, PyObject* args)
{
    const char* path = nullptr;
    if (!PyArg_ParseTuple(args, "s", &path)) return nullptr;
    PyArxQmlSetLogPath(path);
    Py_RETURN_TRUE;
}

static PyObject* py_attach(PyObject*, PyObject* args)
{
    const char* qml = nullptr;
    unsigned long long parent = 0;
    int w = 340, h = 620;
    if (!PyArg_ParseTuple(args, "sKii", &qml, &parent, &w, &h)) return nullptr;

    std::wstring qmlW;
    const size_t n = ::strlen(qml);
    qmlW.resize(n);
    for (size_t i = 0; i < n; ++i) qmlW[i] = (wchar_t)qml[i];

    const bool ok = QmlHost::Instance().Attach(qmlW, (void*)parent, w, h);
    return PyBool_FromLong(ok ? 1 : 0);
}

static PyObject* py_ensure(PyObject*, PyObject*)
{
    return PyBool_FromLong(QmlHost::Instance().Ensure(true) ? 1 : 0);
}

static PyObject* py_ensure_core(PyObject*, PyObject*)
{
    return PyBool_FromLong(QmlHost::Instance().CoreOnlyProbe() ? 1 : 0);
}

static PyObject* py_pump(PyObject*, PyObject*)
{
    QmlHost::Instance().Pump();
    Py_RETURN_NONE;
}

static PyObject* py_stop(PyObject*, PyObject*)
{
    QmlHost::Instance().Stop();
    Py_RETURN_NONE;
}

static PyObject* py_hwnd(PyObject*, PyObject*)
{
    return PyLong_FromLongLong((long long)(unsigned long long)QmlHost::Instance().Hwnd());
}

static PyObject* py_stats(PyObject*, PyObject*)
{
    const HostStats s = QmlHost::Instance().Stats();
    return Py_BuildValue("{s:i,s:L,s:i,s:i,s:i}",
                         "ticks", s.ticks,
                         "max_tick_ms", (long long)s.maxTickMs,
                         "frames", s.frames,
                         "has_app", s.hasApp ? 1 : 0,
                         "has_view", s.hasView ? 1 : 0);
}

static PyObject* py_grab(PyObject*, PyObject* args)
{
    const char* png = nullptr;
    if (!PyArg_ParseTuple(args, "s", &png)) return nullptr;
    const size_t n = ::strlen(png);
    std::wstring pngW;
    pngW.resize(n);
    for (size_t i = 0; i < n; ++i) pngW[i] = (wchar_t)png[i];
    return PyBool_FromLong(QmlHost::Instance().Grab(pngW) ? 1 : 0);
}

static PyObject* py_set_callback(PyObject*, PyObject* args)
{
    PyObject* fn = nullptr;
    if (!PyArg_ParseTuple(args, "O", &fn)) return nullptr;
    Py_XINCREF(fn);
    Py_XDECREF(g_cb);
    g_cb = fn;
    LogWrite("Python 回调已注册");
    Py_RETURN_TRUE;
}

static PyObject* py_push(PyObject*, PyObject* args)
{
    const char* text = nullptr;
    if (!PyArg_ParseTuple(args, "s", &text)) return nullptr;
    return PyBool_FromLong(QmlHost::Instance().PushStatus(text) ? 1 : 0);
}

static PyObject* py_query(PyObject*, PyObject* args)
{
    const char* name = nullptr;
    if (!PyArg_ParseTuple(args, "s", &name)) return nullptr;
    const std::string v = QmlHost::Instance().QueryProp(name);
    if (v.empty()) Py_RETURN_NONE;
    return PyUnicode_FromString(v.c_str());
}

// set_prop(name, value) —— 写 qml 根对象属性
static PyObject* py_set_prop(PyObject*, PyObject* args)
{
    const char* name = nullptr;
    PyObject* value = nullptr;
    if (!PyArg_ParseTuple(args, "sO", &name, &value)) return nullptr;

    QObject* root = (QObject*)QmlHost::Instance().RootObject();
    if (!root) {
        LogWrite("set_prop(%s) 失败：qml 尚未加载", name);
        Py_RETURN_FALSE;
    }
    const QVariant v = FromPy(value);
    const bool ok = WriteProp(root, QString::fromUtf8(name), v);
    LogWrite("Python -> QML: %s = %s（%s）", name, v.toString().toUtf8().constData(),
             ok ? "ok" : "失败");
    return PyBool_FromLong(ok ? 1 : 0);
}

// get_prop(name)
static PyObject* py_get_prop(PyObject*, PyObject* args)
{
    const char* name = nullptr;
    if (!PyArg_ParseTuple(args, "s", &name)) return nullptr;
    QObject* root = (QObject*)QmlHost::Instance().RootObject();
    if (!root) Py_RETURN_NONE;
    return ToPy(ReadProp(root, QString::fromUtf8(name)));
}

// set_named(object_name, prop, value) —— 写 qml 里 objectName 指定的控件
static PyObject* py_set_named(PyObject*, PyObject* args)
{
    const char* objName = nullptr;
    const char* prop = nullptr;
    PyObject* value = nullptr;
    if (!PyArg_ParseTuple(args, "ssO", &objName, &prop, &value)) return nullptr;

    bool found = false;
    QObject* obj = Target(objName, &found);
    if (!obj) {
        LogWrite("set_named(%s) 失败：找不到该 objectName", objName);
        Py_RETURN_FALSE;
    }
    const QVariant v = FromPy(value);
    const bool ok = WriteProp(obj, QString::fromUtf8(prop), v);
    LogWrite("Python -> QML: [%s].%s = %s（%s）", objName, prop,
             v.toString().toUtf8().constData(), ok ? "ok" : "失败");
    return PyBool_FromLong(ok ? 1 : 0);
}

// get_named(object_name, prop)
static PyObject* py_get_named(PyObject*, PyObject* args)
{
    const char* objName = nullptr;
    const char* prop = nullptr;
    if (!PyArg_ParseTuple(args, "ss", &objName, &prop)) return nullptr;
    QObject* obj = Target(objName, nullptr);
    if (!obj) Py_RETURN_NONE;
    return ToPy(ReadProp(obj, QString::fromUtf8(prop)));
}

// has_item(object_name)
static PyObject* py_has_item(PyObject*, PyObject* args)
{
    const char* objName = nullptr;
    if (!PyArg_ParseTuple(args, "s", &objName)) return nullptr;
    PyObject* r = PyBool_FromLong(Target(objName, nullptr) ? 1 : 0);
    return r;
}

// call(method, *args) —— 调 qml 根对象上的 JS 函数，返回它的返回值
static PyObject* py_call(PyObject*, PyObject* args)
{
    if (PyTuple_GET_SIZE(args) < 1) {
        PyErr_SetString(PyExc_TypeError, "call(method, *args)");
        return nullptr;
    }
    PyObject* m = PyTuple_GET_ITEM(args, 0);
    const char* method = PyUnicode_AsUTF8(m);
    if (!method) return nullptr;

    QObject* root = (QObject*)QmlHost::Instance().RootObject();
    if (!root) {
        LogWrite("call(%s) 失败：qml 尚未加载", method);
        Py_RETURN_NONE;
    }
    const QVariantList argv = TupleToArgs(args, 1);
    const QVariant ret = InvokeQml(root, QByteArray(method), argv);
    LogWrite("Python -> QML 调用 %s(%d 个参数) -> %s", method, (int)argv.size(),
             ret.toString().toUtf8().constData());
    PyObject* out = ToPy(ret);
    if (!out) { PyErr_Clear(); Py_RETURN_NONE; }
    return out;
}

// call_named(object_name, method, *args)
static PyObject* py_call_named(PyObject*, PyObject* args)
{
    if (PyTuple_GET_SIZE(args) < 2) {
        PyErr_SetString(PyExc_TypeError, "call_named(object_name, method, *args)");
        return nullptr;
    }
    const char* objName = PyUnicode_AsUTF8(PyTuple_GET_ITEM(args, 0));
    const char* method = PyUnicode_AsUTF8(PyTuple_GET_ITEM(args, 1));
    if (!objName || !method) return nullptr;

    QObject* obj = Target(objName, nullptr);
    if (!obj) {
        LogWrite("call_named(%s.%s) 失败：找不到该 objectName", objName, method);
        Py_RETURN_NONE;
    }
    const QVariantList argv = TupleToArgs(args, 2);
    const QVariant ret = InvokeQml(obj, QByteArray(method), argv);
    LogWrite("Python -> QML 调用 %s.%s(%d 个参数) -> %s", objName, method, (int)argv.size(),
             ret.toString().toUtf8().constData());
    PyObject* out = ToPy(ret);
    if (!out) { PyErr_Clear(); Py_RETURN_NONE; }
    return out;
}

// tree(max_depth, max_nodes) —— 导出整棵 QML 对象树，不用手写 objectName
static PyObject* py_tree(PyObject*, PyObject* args)
{
    int maxDepth = 99;
    int maxNodes = 500;
    if (!PyArg_ParseTuple(args, "|ii", &maxDepth, &maxNodes)) return nullptr;

    QObject* root = (QObject*)QmlHost::Instance().RootObject();
    if (!root) Py_RETURN_NONE;

    QVariantList nodes;
    int budget = maxNodes;
    DumpNode(root, QStringLiteral("0"), 0, maxDepth, nodes, budget);
    return ToPy(QVariant(nodes));
}

// set_path(path, prop, value) —— 按树路径写属性，path 形如 "0/2/1"
static PyObject* py_set_path(PyObject*, PyObject* args)
{
    const char* path = nullptr;
    const char* prop = nullptr;
    PyObject* value = nullptr;
    if (!PyArg_ParseTuple(args, "ssO", &path, &prop, &value)) return nullptr;

    QObject* o = ResolvePath(QString::fromUtf8(path));
    if (!o) {
        LogWrite("set_path(%s) 失败：路径不存在", path);
        Py_RETURN_FALSE;
    }
    const QVariant v = FromPy(value);
    const bool ok = WriteProp(o, QString::fromUtf8(prop), v);
    LogWrite("Python -> QML: [%s].%s = %s（%s）", path, prop,
             v.toString().toUtf8().constData(), ok ? "ok" : "失败");
    return PyBool_FromLong(ok ? 1 : 0);
}

// get_path(path, prop)
static PyObject* py_get_path(PyObject*, PyObject* args)
{
    const char* path = nullptr;
    const char* prop = nullptr;
    if (!PyArg_ParseTuple(args, "ss", &path, &prop)) return nullptr;
    QObject* o = ResolvePath(QString::fromUtf8(path));
    if (!o) Py_RETURN_NONE;
    return ToPy(ReadProp(o, QString::fromUtf8(prop)));
}

// names(max_depth, max_nodes) —— 全量 objectName -> 类型 字典（对齐 PyMfcCore.xaml 的 names()）
static void CollectNames(QObject* o, int depth, int maxDepth, QVariantMap& out, int& budget)
{
    if (!o || budget <= 0) return;
    --budget;
    if (!o->objectName().isEmpty() && !out.contains(o->objectName()))
        out.insert(o->objectName(), FriendlyClass(o));
    if (depth >= maxDepth) return;
    QList<QObject*> kids;
    ChildList(o, kids);
    for (QObject* k : kids)
        CollectNames(k, depth + 1, maxDepth, out, budget);
}

static PyObject* py_names(PyObject*, PyObject* args)
{
    int maxDepth = 99;
    int maxNodes = 2000;
    if (!PyArg_ParseTuple(args, "|ii", &maxDepth, &maxNodes)) return nullptr;

    QObject* root = (QObject*)QmlHost::Instance().RootObject();
    if (!root) Py_RETURN_NONE;

    QVariantMap out;
    int budget = maxNodes;
    CollectNames(root, 0, maxDepth, out, budget);
    LogWrite("names() 导出 %d 个 objectName", (int)out.size());
    return ToPy(QVariant(out));
}

// eval(expr, object_name=None) —— 在 QML 上下文里执行 JS 表达式并取回结果。
// 与 MFCWebView 的 eval(js) 同一语义，区别是这里能拿到返回值。
static PyObject* py_eval(PyObject*, PyObject* args)
{
    const char* expr = nullptr;
    const char* objName = nullptr;
    if (!PyArg_ParseTuple(args, "s|z", &expr, &objName)) return nullptr;

    QQmlEngine* eng = (QQmlEngine*)QmlHost::Instance().Engine();
    if (!eng) {
        LogWrite("eval 失败：qml 尚未加载");
        Py_RETURN_NONE;
    }
    QObject* scope = (objName && *objName) ? Target(objName, nullptr)
                                           : (QObject*)QmlHost::Instance().RootObject();
    if (!scope) {
        LogWrite("eval 失败：找不到作用域对象 %s", (objName && *objName) ? objName : "<root>");
        Py_RETURN_NONE;
    }

    QQmlExpression ex(eng->rootContext(), scope, QString::fromUtf8(expr));
    const QVariant v = ex.evaluate();
    if (ex.hasError())
        LogWrite("eval 出错: %s（表达式：%s）", ex.error().toString().toUtf8().constData(), expr);
    else
        LogWrite("eval(\"%s\") -> %s", expr, v.toString().toUtf8().constData());

    PyObject* out = ToPy(v);
    if (!out) { PyErr_Clear(); Py_RETURN_NONE; }
    return out;
}

// call_path(path, method, *args)
static PyObject* py_call_path(PyObject*, PyObject* args)
{
    if (PyTuple_GET_SIZE(args) < 2) {
        PyErr_SetString(PyExc_TypeError, "call_path(path, method, *args)");
        return nullptr;
    }
    const char* path = PyUnicode_AsUTF8(PyTuple_GET_ITEM(args, 0));
    const char* method = PyUnicode_AsUTF8(PyTuple_GET_ITEM(args, 1));
    if (!path || !method) return nullptr;

    QObject* o = ResolvePath(QString::fromUtf8(path));
    if (!o) {
        LogWrite("call_path(%s.%s) 失败：路径不存在", path, method);
        Py_RETURN_NONE;
    }
    const QVariantList argv = TupleToArgs(args, 2);
    const QVariant ret = InvokeQml(o, QByteArray(method), argv);
    LogWrite("Python -> QML 调用 [%s].%s(%d 个参数) -> %s", path, method, (int)argv.size(),
             ret.toString().toUtf8().constData());
    PyObject* out = ToPy(ret);
    if (!out) { PyErr_Clear(); Py_RETURN_NONE; }
    return out;
}

// diag() —— 环境诊断：Qt 版本、平台、屏幕数、字体库规模与枚举耗时（用于分析构造慢在哪一段）
static PyObject* py_diag(PyObject*, PyObject*)
{
    PyObject* d = PyDict_New();
    const char* ver = qVersion();
    PyDict_SetItemString(d, "qt_version", PyUnicode_FromString(ver ? ver : "?"));
    PyDict_SetItemString(d, "qt_compile", PyUnicode_FromString(QT_VERSION_STR));

    if (!QGuiApplication::instance()) {
        PyDict_SetItemString(d, "has_app", Py_False);
        return d;
    }
    PyDict_SetItemString(d, "has_app", Py_True);
    PyDict_SetItemString(d, "platform",
                         PyUnicode_FromString(QGuiApplication::platformName().toUtf8().constData()));

    QElapsedTimer t;
    t.start();
    const QStringList fam = QFontDatabase::families();
    const long long famMs = t.elapsed();

    t.restart();
    const QList<QScreen*> screens = QGuiApplication::screens();
    const long long scrMs = t.elapsed();

    t.restart();
    QFont f(QStringLiteral("Microsoft YaHei"));
    const bool exact = QFontInfo(f).exactMatch();
    const long long fontMs = t.elapsed();

    PyDict_SetItemString(d, "font_families", PyLong_FromLong((long)fam.size()));
    PyDict_SetItemString(d, "font_families_ms", PyLong_FromLongLong(famMs));
    PyDict_SetItemString(d, "screens", PyLong_FromLong((long)screens.size()));
    PyDict_SetItemString(d, "screens_ms", PyLong_FromLongLong(scrMs));
    PyDict_SetItemString(d, "yahei_exact", exact ? Py_True : Py_False);
    PyDict_SetItemString(d, "yahei_ms", PyLong_FromLongLong(fontMs));

    LogWrite("diag: Qt=%s 平台=%s 字体 %d 个（枚举 %.1f ms）屏幕 %d 个（枚举 %.1f ms）雅黑精确匹配=%d（%.1f ms）",
             ver ? ver : "?", QGuiApplication::platformName().toUtf8().constData(),
             (int)fam.size(), (double)famMs, (int)screens.size(), (double)scrMs,
             exact ? 1 : 0, (double)fontMs);
    return d;
}

// render_backend() —— 场景图后端（Software 说明没有 GPU 合成，整窗重绘都要 CPU 走）
static PyObject* py_render_backend(PyObject*, PyObject*)
{
    const int api = QmlHost::Instance().RenderBackend();
    const char* names[] = { "Unknown", "Software", "OpenGL", "D3D11", "Vulkan", "Metal", "Null" };
    const char* name = (api >= 0 && api <= 6) ? names[api] : "?";
    LogWrite("场景图后端 = %s（%d）", name, api);
    return Py_BuildValue("{s:i,s:s}", "api", api, "name", name);
}

static PyMethodDef g_methods[] = {
    { "set_log",        py_set_log,        METH_VARARGS, "set_log(path)" },
    { "ensure",         py_ensure,         METH_NOARGS,  "ensure() —— 只建 QGuiApplication 和泵，不建面板" },
    { "ensure_core",    py_ensure_core,    METH_NOARGS,  "ensure_core() —— 探针：只建 QCoreApplication，用完必须退出进程" },
    { "diag",          py_diag,           METH_NOARGS,  "diag() —— 环境诊断（Qt 版本/平台/字体/屏幕及枚举耗时）" },
    { "render_backend", py_render_backend, METH_NOARGS,  "render_backend() —— 场景图后端（Software/OpenGL/D3D11…）" },
    { "attach",         py_attach,         METH_VARARGS, "attach(qml_path, parent_hwnd, width, height)" },
    { "pump",           py_pump,           METH_NOARGS,  "pump() —— 手动跑一轮事件循环" },
    { "stop",           py_stop,           METH_NOARGS,  "stop() —— 停泵定时器" },
    { "hwnd",           py_hwnd,           METH_NOARGS,  "hwnd() —— QQuickView 的窗口句柄" },
    { "stats",          py_stats,          METH_NOARGS,  "stats()" },
    { "grab",           py_grab,           METH_VARARGS, "grab(png_path)" },
    { "set_callback",   py_set_callback,   METH_VARARGS, "set_callback(fn) —— fn(op, a, b) -> str" },
    { "push",           py_push,           METH_VARARGS, "push(text) —— 写到 backend.status" },
    { "query",          py_query,          METH_VARARGS, "query(prop) —— 读 backend 的属性" },
    { "set_prop",       py_set_prop,       METH_VARARGS, "set_prop(name, value) —— 写 qml 根对象属性" },
    { "get_prop",       py_get_prop,       METH_VARARGS, "get_prop(name) —— 读 qml 根对象属性" },
    { "set_named",      py_set_named,      METH_VARARGS, "set_named(objectName, prop, value)" },
    { "get_named",      py_get_named,      METH_VARARGS, "get_named(objectName, prop)" },
    { "has_item",       py_has_item,       METH_VARARGS, "has_item(objectName) —— 控件是否存在" },
    { "call",           py_call,           METH_VARARGS, "call(method, *args) —— 调 qml 根对象上的函数" },
    { "call_named",     py_call_named,     METH_VARARGS, "call_named(objectName, method, *args)" },
    { "tree",           py_tree,           METH_VARARGS, "tree(max_depth=99, max_nodes=500) —— 导出 QML 对象树" },
    { "set_path",       py_set_path,       METH_VARARGS, "set_path(path, prop, value) —— 按树路径写属性" },
    { "get_path",       py_get_path,       METH_VARARGS, "get_path(path, prop)" },
    { "call_path",      py_call_path,      METH_VARARGS, "call_path(path, method, *args)" },
    { "names",          py_names,          METH_VARARGS, "names(max_depth=99, max_nodes=2000) —— 全量 {objectName: 类型}" },
    { "eval",           py_eval,           METH_VARARGS, "eval(expr, object_name=None) —— 执行 JS 表达式并返回结果" },
    { nullptr, nullptr, 0, nullptr }
};

static struct PyModuleDef g_module = {
    PyModuleDef_HEAD_INIT,
    "PyArxQml",
    "在 AutoCAD 进程内承载 Qt Quick 面板（QQuickView + 宿主泵 + Python 双向桥）",
    -1,
    g_methods
};

PyMODINIT_FUNC PyInit_PyArxQml(void)
{
    return PyModule_Create(&g_module);
}
