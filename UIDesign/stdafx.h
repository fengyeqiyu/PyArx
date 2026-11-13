#pragma once

#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <fstream>
#include <algorithm>
#include <comdef.h>

#include <msxml6.h>
#pragma comment(lib, "msxml6.lib")

// PyMfcDesigner — 轻量 XML 设计器：
//   * 打开/保存 XML 风格 XML
//   * 左侧控件树，中间预览，右侧属性编辑
//   * 原生 GDI 预览，不依赖 AutoCAD / PyMfc.pyd
