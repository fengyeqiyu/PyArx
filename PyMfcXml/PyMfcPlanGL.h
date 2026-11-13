// 画布 OpenGL 后端（WGL）：只服务配电画布子窗口，其余 UI 仍走原路径。
// 阶段二：纸面 / 连线 / 卡片 / 电压角标 / 文字全部由 GL 画，文字走动态字形图集。
// 默认关闭，PYARX_CANVAS_GL=1 才启用；创建失败自动回退 GDI 绘制。
#pragma once
#include "stdafx.h"
#include <gl/GL.h>
#include "XmlPowerPlan.h"

namespace pymfcgl
{
    bool enabled();

    bool attach(HWND hwnd, int w, int h);
    void detach(HWND hwnd);

    // 画一帧配电图；返回 false 表示没准备好（调用方回退 GDI）
    bool drawPlan(HWND hwnd, const xmlplan::PowerPlan& plan, float k, int ox, int oy,
                  const std::wstring& sel, bool showCable, int w, int h);
}
