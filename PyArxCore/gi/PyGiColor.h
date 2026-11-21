#pragma once
// ===========================================================================
// PyGiColor.h —— AcGi 绑定层需要的 AcCm / AcGi 值类型（原 SDK 缺项补齐）。
//
//   AcCmEntityColor   子实体真彩色（AcGiSubEntityTraits::setTrueColor/trueColor）
//   AcCmTransparency  子实体透明度（setTransparency/transparency）
//   AcGiVariant       AcGiDrawableTraits::setHighlightProperty / highlightProperty
//   AcGiMapper        AcGiSubEntityTraits::setMapper（材质映射，面数据用）
//
// 这四类是 AcGi 方法签名里绕不开的“哑值类型”，全部按值语义绑定在 PyArxCore.Gi
// 子模块（AcCm 两个类型也放这里：Gi 是唯一使用点，Db 侧现有接口仍返回元组）。
// 全部注册必须在任何引用它们的 AcGi 类之前完成。
// ===========================================================================
#include "stdafx.h"

void bind_PyCmEntityColor(pym& m);
void bind_PyCmTransparency(pym& m);
void bind_PyGiVariant(pym& m);
void bind_PyGiMapper(pym& m);
