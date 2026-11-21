// XmlCoreCommon.h —— XmlCore 系列 TU 共用的前置（原 XmlCore.cpp 1-21 行）。
#pragma once

#include "XmlCore.h"
#include "controls/XmlTypes.h"
#include "controls/XmlPropUtil.h"
#include "controls/XmlMfcTags.h"
#include "controls/XmlFlexPanel.h"   // 控件层 flex 容器：模板排版交给它自己的算法

#include <msxml6.h>
#include <algorithm>
#include <cmath>
#include <comdef.h>
#include <cwchar>
#include <cwctype>
#include <fstream>
#include <locale>
#include <map>
#include <set>
#include <string>
#include <typeindex>

#pragma comment(lib, "msxml6.lib")
