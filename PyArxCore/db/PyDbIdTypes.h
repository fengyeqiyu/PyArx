#pragma once

// PyDbIdTypes.h —— PyRx 对齐：Db.ObjectId / Db.Handle（P1）
//
// 直接绑定 ObjectARX 原生类型 AcDbObjectId / AcDbHandle，不套包装类：
//   - 值语义由 SDK 自身提供（AcDbObjectId 隐式拷贝 + operator=）；
//   - 与 PyArx 既有 uint64_t 口径通过 asOldId() / setFromOldId() 互通，零指针强转；
//   - PyRx 独有的 fromOldId() 静态构造、__int__、hash、repr 在绑定层补齐。
#include "stdafx.h"

// 注册到 Db 子模块：PyDbObjectId（别名 ObjectId）/ PyDbHandle（别名 Handle）
void bind_PyDbIdTypes(pym& m);
