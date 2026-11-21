#pragma once

// bind 层：pybind11 浅封装边界（对标 Rust serde 边界）。
// 仅做子模块 / 值类型 / 句柄的转换与导出，不承载解码、几何或交互逻辑。
// 单向依赖：bind 依赖以下各层；rx/ge/db/io/gi/ed/ap 内部禁止 include 本目录。
// 具体绑定随 #17 落地。
