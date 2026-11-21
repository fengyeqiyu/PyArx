#pragma once
#include "stdafx.h"

// Central declarations for all pybind11 enum bindings (embedded module "PyEnum").
// Split into per-cluster PyAds*.h; this umbrella keeps every bind_* declaration
// reachable from the original include path. PyEnum 子模块注册函数 bind_PyEnum
// 定义于 PyEnum.cpp。
#include "PyAdsErrorStatus.h"
#include "PyAdsPromptStatus.h"
#include "PyAdsDbCoreEnums.h"
#include "PyAdsDxfCode.h"
#include "PyAdsLineEnums.h"
#include "PyAdsTableUpdateEnums.h"
#include "PyAdsFindTextOptions.h"
#include "PyAdsJigEnums.h"
#include "PyExtraEnums.h"

// PyEnum 子模块入口，由 PyArxBinder.cpp 调用
void bind_PyEnum(pym& m);
