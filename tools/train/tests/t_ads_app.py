# -*- coding: utf-8 -*-
"""Ads 定位/枚举实测 + App 层只读核对。不调 GUI 弹窗/不发命令字符串。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

SHARD = r"G:\PyArx\tools\train\data\shard_ed_app_engine.json"

def members(obj):
    return [x for x in dir(obj) if not x.startswith("_")]

# ------------------------------------------------------------------ Ads 陷阱定位
AD_TRAPS = {
    "PyAdsErrorStatus": ("ErrorStatus", "Acad::ErrorStatus 错误状态枚举"),
    "PyAdsDxfCode": ("DxfCode", "AcDb::Code DXF 组码枚举"),
    "PyAdsPromptStatus": ("PromptStatus", "Acad::PromptStatus 提示状态枚举"),
}
for trap, (real, note) in AD_TRAPS.items():
    def t(trap=trap, real=real, note=note):
        in_ed = hasattr(P.Ed, trap)
        in_enum = hasattr(P.Enum, real)
        in_other = {m: hasattr(getattr(P, m, None), trap)
                    for m in ("Core", "Rx", "Utility", "Ap", "Db", "Ge", "Gi", "Debug")}
        return "%s 在 P.Ed=%s；跨模块扫描其它位置=%s；真实对应物 P.Enum.%s exists=%s（%s）" % (
            trap, in_ed, in_other, real, in_enum, note)
    mlab.check("Ads", trap, "trap_missing_real_location", t)

# ------------------------------------------------------------------ Ads 枚举实测
def enum_check(mod, cls, samples):
    def t():
        obj = getattr(P.Enum, cls)
        vals = {}
        for s in samples:
            try:
                v = getattr(obj, s)
                vals[s] = {"int": int(v), "repr": repr(v)[:80]}
            except Exception as e:
                vals[s] = "%s: %s" % (type(e).__name__, str(e)[:100])
        return "P.Enum.%s type=%s.%s；样本=%s" % (cls, type(obj).__module__, type(obj).__name__, vals)
    mlab.check("Ads", "Enum." + cls, "exists+enum_values", t)

enum_check("Ads", "ErrorStatus", ["eOk", "eInvalidInput", "eInvalidAdsName"])
enum_check("Ads", "DxfCode", ["kDxfInvalid", "kDxfStart", "kDxfEnd"])
enum_check("Ads", "PromptStatus", ["eNone", "eNormal", "eError", "eCancel", "eKeyword"])
enum_check("Ads", "AdsReturn", ["kNorm", "kError", "kCancel", "kKeyword"])

# AdsName（真实路径 Ed.AdsName）
mlab.check("Ads", "Ed.AdsName", "exists+ctor+members+data", lambda: (
    "members=%s；ctor()=%s.%s data=%r isValid=%s；ctor(1,2)=%r；typeerror(x)=%s" % (
        members(P.Ed.AdsName), type(P.Ed.AdsName()).__module__, type(P.Ed.AdsName()).__name__,
        P.Ed.AdsName().data(), P.Ed.AdsName().isValid(), P.Ed.AdsName(1, 2).data(),
        mlab.typeerror_hint(P.Ed.AdsName, "x"))))

# ------------------------------------------------------------------ App：PyApp 静态门面
def t_pyapp():
    dm = P.Ap.PyApp.documentManager
    return ("静态门面 P.Ap.PyApp；documentManager=%s.%s；mainWindow=%r；statusBar=%s；"
            "acadApplication=%r；静态方法: preferences/infoCenter/menuBar/menuGroups/uiBindings/isInCustomizationMode/displayTextScreen"
            % (type(dm).__module__, type(dm).__name__, P.Ap.PyApp.mainWindow,
               type(P.Ap.PyApp.statusBar).__name__, P.Ap.PyApp.acadApplication))
mlab.check("App", "PyApp", "static_properties_read", t_pyapp)

def t_pyapp_unsupported():
    out = {}
    for meth in ("preferences", "infoCenter", "menuBar", "menuGroups", "uiBindings"):
        try:
            getattr(P.Ap.PyApp, meth)()
            out[meth] = "returned"
        except Exception as e:
            out[meth] = "%s: %s" % (type(e).__name__, str(e)[:120])
    return "无 C++ 等价物成员降级行为: %s" % out
mlab.check("App", "PyApp", "unsupported_members_degrade", t_pyapp_unsupported)

# DocumentManager
def t_manager():
    dm = P.Ap.PyApp.documentManager
    docs = dm.documents
    return ("P.Ap.DocumentManager=%s repr=%r；count=%d；isApplicationContext=%s；documents=%d；"
            "curDocument=%s；mdiActiveDocument=%s" % (
                type(dm).__name__, repr(dm), dm.count, dm.isApplicationContext, len(docs or []),
                type(dm.curDocument).__name__, type(dm.mdiActiveDocument).__name__))
mlab.check("App", "DocumentManager", "count_curdoc_mdiactive", t_manager)

# Document
def t_doc():
    dm = P.Ap.PyApp.documentManager
    doc = dm.curDocument if dm.curDocument is not None else dm.mdiActiveDocument
    return ("P.Ap.Document=%s.%s；name=%r；docTitle=%r；isNamedDrawing=%s；isActive=%s；"
            "isReadOnly=%s；isQuiescent=%s；database=%s；windowHandle=%r" % (
                type(doc).__module__, type(doc).__name__, doc.name, doc.docTitle, doc.isNamedDrawing,
                doc.isActive, doc.isReadOnly, doc.isQuiescent, type(doc.database).__name__, doc.windowHandle))
mlab.check("App", "Document", "name_title_readback", t_doc)

mlab.check("App", "Document", "save_unnamed_known_eInvalidInput_no_retry", lambda: (
    "未命名图形 Document.save() 历史实测返回 eInvalidInput；本轮按口径不再重试。成员签名: save()->ErrorStatus 字符串；saveAs(path)->ErrorStatus；sendStringToExecute(command,...)；activate/lockDocument/unlockDocument/setFocus 不调 GUI 路径"))

# PaletteSet / Palette（GUI 侧，无头只测存在性/构造签名/成员）
mlab.check("App", "PaletteSet", "exists+ctor_hint+members", lambda: (
    "members=%s；typeerror(x)=%s" % (members(P.Ap.PaletteSet), mlab.typeerror_hint(P.Ap.PaletteSet, "x"))))
mlab.check("App", "Palette", "exists+ctor_hint+members", lambda: (
    "members=%s；typeerror(x)=%s" % (members(P.Ap.Palette), mlab.typeerror_hint(P.Ap.Palette, "x"))))
mlab.check("App", "PaletteSet", "headless_gui_note", lambda: (
    "create/show/hide/addPage/pageCount/name 为 MFC 侧栏 GUI 操作，无头 console 无宿主窗口，按口径不触发；成员=%s" % members(P.Ap.PaletteSet)))

rep = mlab.report()
dump = mlab.dump(SHARD)
out = mlab.out_path(r"C:\temp\ads_app.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"report": rep, "dump": dump}, f, ensure_ascii=False, indent=1)
print(json.dumps({"dump": dump, "modules": len(rep)}, ensure_ascii=False))
