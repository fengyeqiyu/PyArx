#pragma once

#include "dwg_reader/rx/object.h"

namespace dwr {

/// 非图形数据库对象基类（蓝本 AcDbObject）。
/// 字典项、表记录、Xrecord、 reactors 等不参与显示的对象继承本类；
/// 图形对象走 db::Entity。
class DbObject : public RxObject {
public:
    ~DbObject() override = default;
    using RxObject::RxObject;
};

}  // namespace dwr
