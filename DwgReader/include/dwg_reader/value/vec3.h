#pragma once

#include "dwg_reader/ge/point3d.h"

namespace dwr {

/// 过渡兼容：Vec3 别名 ge::Point3d，新代码直接用 Point3d。
using Vec3 = Point3d;

}  // namespace dwr
