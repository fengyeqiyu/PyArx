/****************************************************************************
** Meta object code from reading C++ file 'SvgGraphItem.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "SvgGraphItem.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SvgGraphItem.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SvgGraphItem_t {
    uint offsetsAndSizes[22];
    char stringdata0[88];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_SvgGraphItem_t::offsetsAndSizes) + ofs), len 
static const qt_meta_stringdata_SvgGraphItem_t qt_meta_stringdata_SvgGraphItem = {
    {
        QT_MOC_LITERAL(0, 12),  // "SvgGraphItem"
        QT_MOC_LITERAL(13, 10),  // "svgChanged"
        QT_MOC_LITERAL(24, 0),  // ""
        QT_MOC_LITERAL(25, 10),  // "opsChanged"
        QT_MOC_LITERAL(36, 13),  // "offsetChanged"
        QT_MOC_LITERAL(50, 11),  // "zoomChanged"
        QT_MOC_LITERAL(62, 3),  // "svg"
        QT_MOC_LITERAL(66, 3),  // "ops"
        QT_MOC_LITERAL(70, 6),  // "offset"
        QT_MOC_LITERAL(77, 4),  // "zoom"
        QT_MOC_LITERAL(82, 5)   // "valid"
    },
    "SvgGraphItem\0svgChanged\0\0opsChanged\0"
    "offsetChanged\0zoomChanged\0svg\0ops\0"
    "offset\0zoom\0valid"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SvgGraphItem[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       5,   42, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x06,    6 /* Public */,
       3,    0,   39,    2, 0x06,    7 /* Public */,
       4,    0,   40,    2, 0x06,    8 /* Public */,
       5,    0,   41,    2, 0x06,    9 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags
       6, QMetaType::QString, 0x00015103, uint(0), 0,
       7, QMetaType::QString, 0x00015103, uint(1), 0,
       8, QMetaType::QPointF, 0x00015103, uint(2), 0,
       9, QMetaType::QReal, 0x00015103, uint(3), 0,
      10, QMetaType::Bool, 0x00015001, uint(0), 0,

       0        // eod
};

void SvgGraphItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SvgGraphItem *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->svgChanged(); break;
        case 1: _t->opsChanged(); break;
        case 2: _t->offsetChanged(); break;
        case 3: _t->zoomChanged(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SvgGraphItem::*)();
            if (_t _q_method = &SvgGraphItem::svgChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SvgGraphItem::*)();
            if (_t _q_method = &SvgGraphItem::opsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SvgGraphItem::*)();
            if (_t _q_method = &SvgGraphItem::offsetChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SvgGraphItem::*)();
            if (_t _q_method = &SvgGraphItem::zoomChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<SvgGraphItem *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->svg(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->ops(); break;
        case 2: *reinterpret_cast< QPointF*>(_v) = _t->offset(); break;
        case 3: *reinterpret_cast< qreal*>(_v) = _t->zoom(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->valid(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<SvgGraphItem *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setSvg(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setOps(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->setOffset(*reinterpret_cast< QPointF*>(_v)); break;
        case 3: _t->setZoom(*reinterpret_cast< qreal*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
#endif // QT_NO_PROPERTIES
    (void)_a;
}

const QMetaObject SvgGraphItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QQuickPaintedItem::staticMetaObject>(),
    qt_meta_stringdata_SvgGraphItem.offsetsAndSizes,
    qt_meta_data_SvgGraphItem,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_SvgGraphItem_t
, QtPrivate::TypeAndForceComplete<QString, std::true_type>, QtPrivate::TypeAndForceComplete<QString, std::true_type>, QtPrivate::TypeAndForceComplete<QPointF, std::true_type>, QtPrivate::TypeAndForceComplete<qreal, std::true_type>, QtPrivate::TypeAndForceComplete<bool, std::true_type>, QtPrivate::TypeAndForceComplete<SvgGraphItem, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>



>,
    nullptr
} };


const QMetaObject *SvgGraphItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SvgGraphItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SvgGraphItem.stringdata0))
        return static_cast<void*>(this);
    return QQuickPaintedItem::qt_metacast(_clname);
}

int SvgGraphItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QQuickPaintedItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void SvgGraphItem::svgChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SvgGraphItem::opsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SvgGraphItem::offsetChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SvgGraphItem::zoomChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
