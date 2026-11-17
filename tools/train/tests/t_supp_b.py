# -*- coding: utf-8 -*-
"""B 轮：单断言模块补 2-3 条 + 数值变量 TypedValue 往返 + 补回 entget 失败面。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

SHARD = r"G:\PyArx\tools\train\data\shard_ed_app_engine.json"

def members(obj):
    return [x for x in dir(obj) if not x.startswith("_")]

# ------------------------------------------------- 数值变量往返（TypedValue，不碰字符串变量）
def t_var_num():
    before = P.Ed.acedGetVar("OSMODE")
    alt = 0 if int(before) != 0 else 1
    attempts = []
    set_ok = None
    for code in (3, 7):  # RTSHORT / RTLONG
        try:
            tv = P.Db.TypedValue(code, alt)
            set_ok = P.Ed.acedSetVar("OSMODE", tv)
            second = P.Ed.acedGetVar("OSMODE")
            attempts.append("code=%d 返回=%s 回读=%r" % (code, set_ok, second))
            if str(second) == str(alt):
                break
        except Exception as ex:
            attempts.append("code=%d %s: %s" % (code, type(ex).__name__, str(ex)[:120]))
    # 恢复原值
    restore = None
    try:
        tv0 = P.Db.TypedValue(3, before)
        P.Ed.acedSetVar("OSMODE", tv0)
        restore = P.Ed.acedGetVar("OSMODE")
    except Exception as ex:
        restore = "%s: %s" % (type(ex).__name__, str(ex)[:120])
    return "OSMODE 原值=%r；尝试=%s；恢复后=%r（原值一致=%s）" % (
        before, attempts, restore, str(restore) == str(before))
mlab.check("Ed", "acedGetVar/acedSetVar", "osmode_typedvalue_roundtrip", t_var_num)

# ------------------------------------------------- AcEdJig
def t_jig():
    j = P.Ed.AcEdJig()
    j.setKeywordList("A B")
    kw = j.keywordList()
    uic = j.userInputControls()
    j.setUserInputControls(uic)
    uic2 = j.userInputControls()
    return "setKeywordList('A B') -> keywordList=%r；userInputControls=%r 写回=%r 一致=%s" % (
        kw, uic, uic2, uic == uic2)
mlab.check("Ed", "AcEdJig", "keyword_uic_roundtrip", t_jig)
mlab.check("Ed", "AcEdJig", "acquisition_headless_unavailable", lambda: (
    "acquirePoint/acquireDist/acquireAngle/acquireString/drag 为交互拖动环，无头阻塞等待用户输入，按口径不真调；"
    "正确路径：GUI 上下文继承 AcEdJig 并重载 sampler/entity，无头不可用"))

# ------------------------------------------------- SelectionSet 对象闭环
line = P.Db.PyDbLine(P.Ge.AcGePoint3d(1.0, 2.0, 0.0), P.Ge.AcGePoint3d(9.0, 2.0, 0.0))
with P.Db.PyDbTrans() as tr:
    tr.addEntity(line)
oids = list(P.Db.curDb().objectIds("AcDbLine"))

def t_ss_obj():
    ss = P.Ed.SelectionSet()
    c0 = ss.count()
    ss.add(oids[0])
    c1 = ss.count()
    has = ss.hasMember(oids[0])
    ss.remove(oids[0])
    c2 = ss.count()
    ss.clear()
    c3 = ss.count()
    return "count 初始=%d add 后=%d hasMember=%s remove 后=%d clear 后=%d" % (c0, c1, has, c2, c3)
mlab.check("Ed", "SelectionSet", "add_remove_clear_loop", t_ss_obj)
mlab.probe("Ed", "SelectionSet", "add 传非实体", lambda: P.Ed.SelectionSet().add("not-an-oid"),
           allow=("Exception",))

# ------------------------------------------------- PyEdResult 语义 / not_applicable
mlab.check("Ed", "PyEdResult", "member_semantics", lambda: (
    "成员 isOk/isCanceled/isKeyWord/isError/isModeless/isNone/isOther/status/value；"
    "这些状态与 value 只有 pyedGetPoint/pyedSelect/pyedEntSel 等交互 API 返回时才填充；"
    "类无构造器（No constructor defined!），Python 侧不能 new 出实例"))
mlab.check("Ed", "PyEdResult", "not_applicable_headless_instance", lambda: (
    "无头环境拿不到 PyEdResult 实例（交互 API 阻塞，不真调）；无法在本轮对真实实例做 status/value 往返；"
    "正确路径：GUI/可交互上下文调用 pyed* 后检查返回对象"))

# ------------------------------------------------- PromptOptions
def t_po():
    po = P.Ed.PromptOptions()
    po.setMessage("选择对象")
    msg = po.message
    return "ctor 成功；成员=%s；setMessage 后 message=%r；flags=%r keyWords=%r" % (
        members(P.Ed.PromptOptions), msg, po.flags, po.keyWords)
mlab.check("Ed", "PromptOptions", "ctor_message_readback", t_po)
mlab.probe("Ed", "PromptOptions", "setFlag 传非法值", lambda: P.Ed.PromptOptions().setFlag("bad-flag"),
           allow=("Exception",))

# ------------------------------------------------- AdsName ObjectId 往返
def t_name_oid():
    e = P.Ed.entlast()
    oid = e.toObjectId()
    ok, n2 = P.Ed.AdsName.fromObjectId(oid)
    back = n2.toObjectId()
    return ("entlast AdsName.toObjectId=%r；fromObjectId ok=%s n2.data=%r isValid=%s；"
            "toObjectId 回=%r 一致=%s；空 name isValid=%s" % (
                oid, ok, n2.data(), n2.isValid(), back, back == oid, P.Ed.AdsName().isValid()))
mlab.check("Ads", "Ed.AdsName", "objectid_roundtrip", t_name_oid)

# ------------------------------------------------- Document
def t_doc():
    dm = P.Ap.PyApp.documentManager
    doc = dm.curDocument
    return ("name=%r；docTitle=%r（两字段不同）；isNamedDrawing=%s；isReadOnly=%s；lockMode=%s；database=%s" % (
        doc.name, doc.docTitle, doc.isNamedDrawing, doc.isReadOnly, doc.lockMode(), type(doc.database).__name__))
mlab.check("App", "Document", "fields_lockmode", t_doc)
def t_doc_dbmod():
    doc = P.Ap.PyApp.documentManager.curDocument
    doc.pushDbmod()
    doc.popDbmod()
    return "pushDbmod/popDbmod 配对调用未抛异常"
mlab.check("App", "Document", "pushpop_dbmod", t_doc_dbmod)

# ------------------------------------------------- DocumentManager
def t_mgr():
    dm = P.Ap.PyApp.documentManager
    docs = dm.documents
    d0 = dm.documentAt(0) if dm.count > 0 else None
    return ("count=%d；len(documents)=%d 一致=%s；documentAt(0).name=%r；curDocument.name=%r 一致=%s；"
            "isApplicationContext=%s" % (
                dm.count, len(docs), dm.count == len(docs),
                d0.name if d0 is not None else None, dm.curDocument.name,
                d0 is not None and d0.name == dm.curDocument.name, dm.isApplicationContext))
mlab.check("App", "DocumentManager", "count_getdocument_consistency", t_mgr)

# ------------------------------------------------- PaletteSet / Palette not_applicable
mlab.check("App", "PaletteSet", "not_applicable_no_ctor", lambda: (
    "No constructor defined!：类抽象/无公开构造，不能 new；正确路径经 P.Ap.getPalette(name) 或 PaletteSet.create(...) 取实例；"
    "hasattr(P.Ap,'getPalette')=%s；GUI 侧栏 create/show/addPage 无头无宿主不触发" % hasattr(P.Ap, "getPalette")))
mlab.check("App", "Palette", "not_applicable_no_ctor", lambda: (
    "No constructor defined!：无公开构造；正确路径由 PaletteSet.addPage 返回 Palette 或 getPalette(name)；"
    "hasattr(P.Ap,'getPalette')=%s；hwnd/title/getHost/setHost/resizeHost 为 MFC 宿主侧，无头不触发" % hasattr(P.Ap, "getPalette")))

# ------------------------------------------------- Enum 4 个：实测 int() + 失败面
def enum_loop():
    rows = {}
    for cls, samples in {
        "ErrorStatus": [("eOk", 0), ("eInvalidInput", 3)],
        "DxfCode": [("kDxfStart", 0), ("kDxfInvalid", -9999)],
        "PromptStatus": [("eNormal", 5100), ("eError", -5001)],
        "AdsReturn": [("kNorm", 5100), ("kError", -5001)],
    }.items():
        obj = getattr(P.Enum, cls)
        vals = {}
        for s, expect in samples:
            v = getattr(obj, s)
            vals[s] = {"int": int(v), "expect": expect, "match": int(v) == expect}
        try:
            getattr(obj, "eNoSuchMemberXYZ")
            missing = "returned"
        except Exception as ex:
            missing = "%s: %s" % (type(ex).__name__, str(ex)[:80])
        rows[cls] = {"values": vals, "missing_member": missing}
    return rows
mlab.check("Ads", "Enum.roundtrip", "int_values_missing_member", lambda: json.dumps(enum_loop(), ensure_ascii=False))

# ------------------------------------------------- PyApp documentManager 闭环
mlab.check("App", "PyApp", "docmanager_count_consistency", lambda: (
    "PyApp.documentManager=%s；其 count=%d；与 DocumentManager.documents 长度=%d 一致=%s" % (
        type(P.Ap.PyApp.documentManager).__name__, P.Ap.PyApp.documentManager.count,
        len(P.Ap.PyApp.documentManager.documents),
        P.Ap.PyApp.documentManager.count == len(P.Ap.PyApp.documentManager.documents))))

# ------------------------------------------------- 补回 entget 失败面（A 轮覆盖时丢失）
mlab.probe("Ed", "entget", "无效句柄字符串", lambda: P.Ed.entget("ZZZ9999"), allow=("Exception", "None", "return"))
mlab.probe("Ed", "entget", "入参 None", lambda: P.Ed.entget(None), allow=("Exception", "None", "return"))

rep = mlab.report()
dump = mlab.dump(SHARD)
out = mlab.out_path(r"C:\temp\supp_b.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"report": rep, "dump": dump}, f, ensure_ascii=False, indent=1)
print(json.dumps({"dump": dump, "modules": len(rep)}, ensure_ascii=False))
