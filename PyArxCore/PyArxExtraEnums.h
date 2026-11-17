#pragma once
// 生成物：tools/gen_extra_enums.py 产出，请勿手改；改法 = 改生成器后重跑。
// 内容：PyRx 有、PyArx 原本没有的枚举类（值取自 PyRx 桩里的真实数值）。
#include <pybind11/pybind11.h>

void bind_PyArxExtraEnums(pybind11::module_& root);
