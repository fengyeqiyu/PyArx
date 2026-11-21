// PyMfcHost.cpp —— 已按职责拆分，本文件仅作目录说明，不承载实现。
// 实现分布：PyMfcHost.Base.cpp（公共前置与文件级辅助） / PyMfcHost.HostBase.cpp（生命周期与宿主底座）
//          PyMfcHost.Procs.cpp（子类化窗口过程） / PyMfcHost.Paint.cpp（自绘）
//          PyMfcHost.Layout.cpp（布局与重排） / PyMfcHost.Controls.cpp（建控件与取值）
//          PyMfcHost.Dwg.cpp（DWG/3D/功能区/文件页） / PyMfcHost.Anim.cpp（动效与滚动）
//          PyMfcHost.Interop.cpp（Python 回调与事件派发）
// 共享内部声明见 PyMfcHostInternal.h（命名空间 pmh，不属于公开接口）。