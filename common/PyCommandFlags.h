#pragma once
// PyCommandFlags.h — 命令标志枚举（CommandFlags）。
// 归属 PyArxCore（Enum 模块绑定处）。PyCmd 类迁至 PyArxEngine 后仍需此定义，
// 故从 PyRxCmd.h 抽出为自足公共头，PyArxCore 与 PyArxEngine 共用。
// 编码：UTF-8 带 BOM。

//------------------------------------------------------------------------------------------------
// 命令标志
enum CommandFlags {
    Modal = 0,
    Transparent = 1,
    UsePickSet = 2,
    Redraw = 4,
    NoPerspective = 8,
    NoMultiple = 0x10,
    NoTileMode = 0x20,
    NoPaperSpace = 0x40,
    NoOem = 0x100,
    Undefined = 0x200,
    InProgress = 0x400,
    Defun = 0x800,
    NoNewStack = 0x10000,
    NoInternalLock = 0x20000,
    DocReadLock = 0x80000,
    DocExclusiveLock = 0x100000,
    Session = 0x200000,
    Interruptible = 0x400000,
    NoHistory = 0x800000,
    NoUndoMarker = 0x1000000,
    NoBlockEditor = 0x2000000,
    NoActionRecording = 0x4000000,
    ActionMacro = 0x8000000,
    NoInferConstraint = 0x40000000
};
