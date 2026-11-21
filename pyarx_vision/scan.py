# -*- coding: utf-8 -*-
"""pyarx_vision.scan —— 图纸全量扫描器（在 CAD 进程内运行）

在 accoreconsole / acad 里通过 PYRUNFILE 执行，把整张 dwg 的模型空间读成一份
中立 JSON，供 pyarx_vision 的认图（frames）与识图（tablemode/recognize）使用。

与旧 tools\cadmcp\sys_export.py 的关键差异（都是本图踩出来的）：
  1. 块名必须走 PyDbBlockTableRecord.getName()；PyDbBlockReference 没有 name()
  2. 块定义去重缓存：同名块只读一次 entityIds()，避免 1.1 万次重复遍历
  3. 块属性 ATTRIB 单独读（设备编号常放在属性里，不在 TEXT 里）
  4. ACAD_TABLE 读单元格文字（表格法系统图/设备表的数据源）
  5. 文字带 textStyle 名、widthFactor、oblique —— 重绘时要像 CAD 那样摆正
  6. 图框由闭合大矩形给出，不写死坐标

用法（CAD 内）：
    (command "PYRUNFILE" "G:/PyArx/pyarx_vision/scan.py")
输出路径由 OUTPUT 常量与 _argv 决定；默认 C:\\temp\\scan.json。
"""
import json
import PyArxCore as P

OUTPUT = r"C:\temp\scan.json"

# DXFOUT 短名 → 类名（dxfName() 返回短名，必须映射）
MAP = {"LINE": "AcDbLine", "LWPOLYLINE": "AcDbPolyline", "POLYLINE": "AcDb2dPolyline",
       "TEXT": "AcDbText", "MTEXT": "AcDbMText", "CIRCLE": "AcDbCircle",
       "ARC": "AcDbArc", "INSERT": "AcDbBlockReference", "MINSERT": "AcDbMInsertBlock",
       "SPLINE": "AcDbSpline", "ELLIPSE": "AcDbEllipse", "POINT": "AcDbPoint",
       "ACAD_TABLE": "AcDbTable", "HATCH": "AcDbHatch"}

_TEXTMAX = 200  # 每个块定义最多留多少条样本文本

# Python 包装类名 → DXF 短名。本版 PyArx 的 ObjectId.dxfName() 会抛异常，
# 只能靠包装类名反查，否则 ATTDEF/ATTRIB/INSERT 全被当成未知对象跳过。
TMAP = {"PyDbLine": "LINE", "PyDbPolyline": "LWPOLYLINE", "PyDb2dPolyline": "POLYLINE",
        "PyDbText": "TEXT", "PyDbMText": "MTEXT", "PyDbCircle": "CIRCLE",
        "PyDbArc": "ARC", "PyDbBlockReference": "INSERT", "PyDbMInsertBlock": "MINSERT",
        "PyDbAttributeDefinition": "ATTDEF", "PyDbAttribute": "ATTRIB",
        "PyDbSpline": "SPLINE", "PyDbEllipse": "ELLIPSE", "PyDbPoint": "POINT",
        "PyDbTable": "ACAD_TABLE", "PyDbHatch": "HATCH"}


def _dxftype(bo, beid):
    """实体短名：先按 Python 包装类名，再退回 ObjectId/对象上的 dxfName。"""
    t = TMAP.get(type(bo).__name__, "")
    if t:
        return t
    for src in (beid, bo):
        try:
            t = str((beid if src is beid else bo).dxfName() or "")
        except Exception:
            t = ""
        if t:
            return t
    return ""


def _clean(s):
    for a, b in (("%%c", "φ"), ("%%C", "φ"), ("%%d", "°"), ("%%D", "°"),
                 ("%%p", "±"), ("%%P", "±"), ("%%%", "%")):
        s = s.replace(a, b)
    return s


def _num(o, names, default=None):
    for n in names:
        if hasattr(o, n):
            try:
                return getattr(o, n)()
            except Exception:
                pass
    return default


def _pt(o, names):
    for n in names:
        if hasattr(o, n):
            try:
                p = getattr(o, n)()
                if p is not None:
                    return (float(p.x), float(p.y))
            except Exception:
                pass
    return None


def _bbox(o):
    try:
        e = o.getGeomExtents()
        return [round(e[0].x, 2), round(e[0].y, 2), round(e[1].x, 2), round(e[1].y, 2)]
    except Exception:
        return None


def _str(o, names):
    for n in names:
        if hasattr(o, n):
            try:
                return str(getattr(o, n)())
            except Exception:
                pass
    return ""


_DBG = []   # 预留的探测记录通道（随 out["err"] 带回）


def _layer_color(lr):
    """图层色。PyArx 的 layerTableRecord.color() 返回 (colorIndex, r, g, b) 元组——
    之前按 AcCmColor 对象去调 colorIndex()/red() 全抛异常、回落成 7，
    于是 53 个图层全白、所有 BYLAYER 图元跟着一起刷白。"""
    try:
        cc = lr.color()
    except Exception:
        return 7
    if isinstance(cc, (tuple, list)):
        try:
            idx = int(cc[0])
            if 1 <= idx <= 255:
                return idx
        except Exception:
            pass
        try:
            if len(cc) >= 4 and (int(cc[1]), int(cc[2]), int(cc[3])) != (0, 0, 0):
                return [int(cc[1]), int(cc[2]), int(cc[3])]
        except Exception:
            pass
        return 7
    # 兼容 AcCmColor 对象形态
    try:
        idx = int(cc.colorIndex())
        if idx not in (0, 7, 256):
            return idx
    except Exception:
        pass
    try:
        r, g, b = int(cc.red()), int(cc.green()), int(cc.blue())
        if (r, g, b) != (0, 0, 0):
            return [r, g, b]
    except Exception:
        pass
    return 7


def _m_apply(m, x, y):
    """仿射 (a,b,c,d,e,f) 作用到点。"""
    return (m[0] * x + m[1] * y + m[4], m[2] * x + m[3] * y + m[5])


def _m_mul(m, n):
    """先 n 再 m 的复合矩阵。"""
    return (m[0] * n[0] + m[1] * n[2], m[0] * n[1] + m[1] * n[3],
            m[2] * n[0] + m[3] * n[2], m[2] * n[1] + m[3] * n[3],
            m[0] * n[4] + m[1] * n[5] + m[4], m[2] * n[4] + m[3] * n[5] + m[5])


def _m_scale(m):
    """矩阵的平均缩放（字高要跟着块放大缩小）。"""
    return max(1e-9, ((m[0] ** 2 + m[2] ** 2) ** 0.5 + (m[1] ** 2 + m[3] ** 2) ** 0.5) / 2.0)


def _walk_attr(tr, btr, sink, depth, m=None, seen=None):
    """递归块定义，直接读属性块实例的 ATTRIB：attributeIds → openAttribute → textString。
    position()/height() 给的是所在块定义坐标系里的值，必须乘累积变换矩阵才是世界坐标/字高。"""
    if depth > 8 or len(sink) > 26000:
        return
    seen = seen if seen is not None else {}
    m = m if m is not None else (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    try:
        eids = btr.entityIds() or []
    except Exception:
        return
    for beid in eids:
        try:
            o = tr.openObject(beid, 0)
        except Exception:
            continue
        try:
            ads = list(o.attribData()) if hasattr(o, "attribData") else []
        except Exception:
            ads = []
        for d in ads:
            try:
                txt = str(d.get("txt") or "").strip()
                if not txt or d.get("inv"):
                    continue
                px = float(d.get("x") or 0.0)
                py = float(d.get("y") or 0.0)
                ha = int(d.get("ha") or 0)
                if ha:
                    # 非左对齐时真正的绘制点是 alignmentPoint（attach 点）
                    ax = float(d.get("ax") or 0.0)
                    ay = float(d.get("ay") or 0.0)
                    if ax or ay:
                        px, py = ax, ay
                wx, wy = _m_apply(m, px, py)
                hh = float(d.get("h") or 0.0)
                if hh <= 0:
                    continue
                # 字高定义在块定义坐标系里，乘当前实例缩放才是图面字高
                hh = hh * _m_scale(m)
                sink.append([round(wx, 2), round(wy, 2), round(hh, 2),
                             round(float(d.get("rot") or 0.0), 4), ha,
                             int(d.get("ci") or 256), txt[:24],
                             int(d.get("va") or 0), str(d.get("ly") or "0")])
                if len(sink) > 26000:
                    return
            except Exception:
                continue
        if not hasattr(o, "blockTableRecordId"):
            continue
        try:
            bid = o.blockTableRecordId()
            if not bid:
                continue
            sub = tr.openObject(bid, 0)
            if sub is None:
                continue
            try:
                sn = str(sub.getName())
            except Exception:
                # xref / 匿名块 getName 会抛，抛了也不能整棵子树跳过
                sn = "b" + str(bid)
            # 平移必须取 position()：blockTransform() 的线性部分可用，但平移位恒为 0，
            # 拿它当平移会让整排轴号叠到块原点、飞到画面外。
            pp = _pt(o, ("position", "location")) or (0.0, 0.0)
            tm = o.blockTransform().toNumpy()
            sm = (float(tm[0][0]), float(tm[0][1]), float(tm[1][0]), float(tm[1][1]),
                  float(pp[0]), float(pp[1]))
        except Exception:
            continue
        # 按「实例位置+块名」去重：同一种块被摆放多次时，每个实例的属性值、
        # 变换矩阵都不同，按块名去重会把第 2 个及以后的轴号整片丢掉。
        # 用实例句柄/位置做 key，块自引用由 depth 上限兜住。
        k = (sn, round(sm[4], 1), round(sm[5], 1))
        if k in seen:
            continue
        seen[k] = 1
        _walk_attr(tr, sub, sink, depth + 1, _m_mul(m, sm), seen)


def _scan_bdef(tr, btr, seen=None, depth=0):
    """按引用链补读一个块定义的几何（只取线/多段线/圆/文字/嵌套引用）。

    主循环只按顶层 INSERT 读定义，块定义内部引用的匿名块（`$TwtSys$` 图签表格、
    `*U` 动态块）从不作为顶层参照出现，因此永远读不到——A1 图框里大片内容
    就是这样整块丢的。这个函数供补读段递归调用。
    """
    bid = btr.entityIds() or []
    types, texts = {}, []
    gl, gp, gc, gt, gi = [], [], [], [], []

    # 注意：这里不放 ATTDEF 的默认值。ATTDEF 是“定义”，真实文字在每个实例的
    # ATTRIB 里（轴号 1…9/A/E/H 就是这样）；按定义画默认值会和实例值重复，
    # 还会被实例矩阵放大成巨字。实例值在 _walk_attr 里按 attribData() 采。
    for beid in bid:
        try:
            bo = tr.openObject(beid, 0)
        except Exception:
            continue
        # dxfName 优先问 ObjectId：实体包装类缺失时 bo.dxfName() 会给空（AcDbObject），
        # 而 ObjectId.dxfName() 直接来自类注册表，能拿到真实的 ATTDEF/ATTRIB 等。
        bt = _dxftype(bo, beid)
        types[bt] = types.get(bt, 0) + 1
        bci, bly = 256, "0"
        try:
            bci = int(bo.colorIndex())
        except Exception:
            pass
        try:
            bly = str(bo.layer())
        except Exception:
            pass
        try:
            bcls = MAP.get(bt, bt)
            if bcls == "AcDbLine" and len(gl) < 90000:
                a = _pt(bo, ("startPoint", "getStartPoint"))
                b2 = _pt(bo, ("endPoint", "getEndPoint"))
                if a and b2:
                    gl.append([round(a[0], 1), round(a[1], 1),
                               round(b2[0], 1), round(b2[1], 1), bci, bly])
            elif bcls in ("AcDbPolyline", "AcDb2dPolyline") and len(gp) < 24000:
                pts = []
                for i in range(int(_num(bo, ("numVerts",), 0) or 0)):
                    q = None
                    for fn in ("getPointAt", "getPointAtParam"):
                        if hasattr(bo, fn):
                            try:
                                q = getattr(bo, fn)(i)
                                break
                            except Exception:
                                pass
                    if q is not None:
                        pts.append([round(q.x, 1), round(q.y, 1)])
                if len(pts) >= 2:
                    gp.append({"p": pts, "c": bool(_num(bo, ("isClosed",), False)),
                               "ci": bci, "ly": bly})
            elif bcls == "AcDbCircle" and len(gc) < 9000:
                c0 = _pt(bo, ("center", "getCenter"))
                r = float(_num(bo, ("radius",), 0.0) or 0.0)
                if c0:
                    gc.append([round(c0[0], 1), round(c0[1], 1), round(r, 1), bci, bly])
            elif bcls in ("AcDbText", "AcDbMText") and len(gt) < 4000:
                s = _clean(_str(bo, ("textString", "contents", "text")))
                pos = _pt(bo, ("position", "getPosition", "location"))
                if s.strip() and pos:
                    gt.append({"x": round(pos[0], 1), "y": round(pos[1], 1), "t": s[:24],
                               "h": round(float(_num(bo, ("height", "textHeight"), 250.0) or 250.0), 1),
                               "r": round(float(_num(bo, ("rotation",), 0.0) or 0.0), 3),
                               "wf": round(float(_num(bo, ("widthFactor",), 1.0) or 1.0), 2),
                               "ha": int(_num(bo, ("horizontalMode",), 0) or 0),
                               "ci": bci, "ly": bly})
            elif bcls in ("", "AcDbAttributeDefinition") and len(gt) < 4000:
                # openObject 对这种实体只给了基类 PyDbObject（className 报 AcDbObject、
                # dxfName 为空），但它有 cast()，先 cast 到真实类再读文字。
                real, realname = bo, ""
                # 只认 Text/MText/PlaceHolder。绝不能把 AttributeDefinition / Attribute 算进来：
                # 块定义里的 ATTDEF 只是“属性占位与默认值”，块参照上的 ATTRIB（attribData）才是
                # 真实文字。两者都收就会出现同一位置两套字，叠在一起（用户说的“画重了”）。
                for cn in ("PyDbText", "PyDbMText", "PyDbPlaceHolder"):
                    cls = getattr(P.Db, cn, None)
                    if cls is None:
                        continue
                    try:
                        c2 = bo.cast(cls)
                    except Exception:
                        c2 = None
                    if c2 is not None:
                        real, realname = c2, cn
                        break
                if len(_DBG) < 8:
                    # xData(appName) 才取得到：先问这个对象挂了哪些 app 名
                    try:
                        ap = real.appName()
                        _DBG.append("APP:%s" % (repr(ap)[:220],))
                        lst = list(ap) if isinstance(ap, (list, tuple)) else ([ap] if ap else [])
                        for an in lst[:4]:
                            try:
                                xd = real.xData(an)
                                _DBG.append("XD2:%s|%s" % (an, repr(xd)[:260]))
                            except Exception as e6:
                                _DBG.append("XD2X:%s|%s" % (an, str(e6)[:50]))
                    except Exception as e7:
                        _DBG.append("APPX:%s" % str(e7)[:70])
                if len(_DBG) < 6:
                    # 天正把轴号这类数据放在扩展数据里：xData() 返回的正是 DXF 组码缓冲
                    try:
                        xdm = getattr(real, "xData", None)
                        xd = None
                        if xdm is not None:
                            for args in ((), (None,)):
                                try:
                                    xd = xdm(*args)
                                    break
                                except Exception:
                                    xd = None
                        _DBG.append("XD:%d|%s" % (len(xd) if xd is not None else -1, repr(xd)[:300]))
                    except Exception as e4:
                        _DBG.append("XDEXC:%s" % str(e4)[:70])
                    try:
                        bd2 = real.getBinaryData()
                        _DBG.append("BIN:%d|%s" % (len(bd2) if bd2 else -1, repr(bd2)[:160]))
                    except Exception:
                        pass
                s = _clean(_str(real, ("textString", "contents", "tag", "tagString", "text")))
                pos = _pt(real, ("position", "getPosition", "alignmentPoint"))
                if s.strip() and len(s.strip()) <= 8 and pos:
                    gt.append({"x": round(pos[0], 1), "y": round(pos[1], 1), "t": s[:8],
                               "h": round(float(_num(real, ("height", "textHeight"), 250.0) or 250.0), 1),
                               "r": round(float(_num(real, ("rotation",), 0.0) or 0.0), 3),
                               "wf": 1.0,
                               "ha": int(_num(real, ("horizontalMode",), 1) or 1),
                               "ci": bci, "ly": bly})
                else:
                    # 天正轴号是代理对象，读不出 textString 时把它拆开，取里面的文字实体
                    try:
                        parts = bo.explode() if hasattr(bo, "explode") else ()
                        for pa in list(parts)[:12]:
                            s2 = _clean(_str(pa, ("textString", "contents", "text")))
                            p2 = _pt(pa, ("position", "getPosition", "alignmentPoint"))
                            h2 = float(_num(pa, ("height", "textHeight"), 0.0) or 0.0)
                            if s2.strip() and p2 and h2 > 0 and len(gt) < 4000:
                                gt.append({"x": round(p2[0], 1), "y": round(p2[1], 1), "t": s2[:8],
                                           "h": round(h2, 1),
                                           "r": round(float(_num(pa, ("rotation",), 0.0) or 0.0), 3),
                                           "wf": 1.0,
                                           "ha": int(_num(pa, ("horizontalMode",), 1) or 1),
                                           "ci": bci, "ly": bly})
                    except Exception:
                        pass
            elif bcls in ("AcDbBlockReference", "AcDbMInsertBlock") and len(gi) < 9000:
                sn, sub_id = "?", None
                try:
                    sub_id = bo.blockTableRecordId()
                    sn = str(tr.openObject(sub_id, 0).getName())
                except Exception:
                    pass
                p = _pt(bo, ("position", "location"))
                s1, s2 = 1.0, 1.0
                try:
                    sf2 = bo.scaleFactors()
                    s1 = round(float(sf2[0]), 3)
                    s2 = round(float(sf2[1]), 3)
                except Exception:
                    pass
                if p:
                    sg = {"n": sn, "p": [round(p[0], 1), round(p[1], 1)],
                          "r": round(float(_num(bo, ("rotation",), 0.0) or 0.0), 4),
                          "s": s1, "sy": s2, "ci": bci, "ly": bly}
                    try:
                        tm2 = bo.blockTransform().toNumpy()
                        sg["m"] = [round(float(tm2[0][0]), 6), round(float(tm2[0][1]), 6),
                                   round(float(tm2[1][0]), 6), round(float(tm2[1][1]), 6),
                                   round(float(p[0]), 3), round(float(p[1]), 3)]
                    except Exception:
                        pass
                    # 属性块实例的值：轴号圈里的 "1"/"A" 就是 ATTRIB，不是 TEXT。
                    # 用 attribData()：attributeIds()+openAttribute() 在块定义内部的
                    # 嵌套 INSERT 上拿不到值，这一层是轴号圈空白的直接原因。
                    try:
                        ads = list(bo.attribData()) if hasattr(bo, "attribData") else []
                        atx2 = []
                        for av in ads[:24]:
                            d2 = dict(av)
                            val = _clean(str(d2.get("txt") or ""))
                            if not val.strip() or d2.get("inv"):
                                continue
                            hh = float(d2.get("h") or 0.0)
                            if hh <= 0:
                                continue
                            # 与 tx 完全同构：[x,y,h,rot,ha,ci,txt,li,va]
                            atx2.append([round(float(d2.get("x") or 0.0), 2),
                                         round(float(d2.get("y") or 0.0), 2),
                                         round(hh, 2),
                                         round(float(d2.get("rot") or 0.0), 4),
                                         int(d2.get("ha") or 0),
                                         int(d2.get("ci") or 256),
                                         val, str(d2.get("ly") or "0"),
                                         int(d2.get("va") or 0)])
                        if atx2:
                            sg["atx"] = atx2
                    except Exception:
                        pass
                    # 记块参照 handle：DXF 里 ATTRIB 按 INSERT 的 handle 归属，
                    # 渲染端要用它把属性值精确配到这一个实例上。
                    try:
                        sg["h"] = str(bo.handle()).upper()
                    except Exception:
                        pass
                    gi.append(sg)
                    if seen is not None and sub_id is not None and depth > 0 and sn not in seen:
                        # ObjectId 一离开当前打开上下文就打不开（openObject 返回 None），
                        # 所以必须在这里当场递归读子块，不能延后到扫描结束再补。
                        try:
                            sb = tr.openObject(sub_id, 0)
                            seen[sn] = (_scan_bdef(tr, sb, seen, depth - 1)
                                        if sb is not None else {"err": "openObject None"})
                        except Exception as ex:
                            seen[sn] = {"err": str(ex)[:60]}
        except Exception:
            pass
        if bt in ("TEXT", "MTEXT", "ATTRIB") and len(texts) < _TEXTMAX:
            txt = _clean(_str(bo, ("textString", "contents")))
            if txt.strip():
                texts.append(txt.strip()[:48])
    return {"n": len(bid), "types": types, "texts": texts, "b": _bbox(btr),
            "geom": {"lines": gl, "plines": gp, "circles": gc, "arcs": [],
                     "texts": gt, "ins": gi}}


def run():
    db = P.Db.curDb()
    out = {"file": "", "frame": None,
           "lines": [], "plines": [], "texts": [], "circles": [], "arcs": [],
           "blocks": [], "tables": [], "block_defs": {}, "styles": {},
           "layers": {}, "layer_col": {}, "layer_lw": {}, "lwd": 0,
           "counts": {}, "err": [], "frame_cands": [], "attrib": []}
    try:
        out["file"] = str(db.getFilename())
    except Exception:
        pass
    # $LWDISPLAY：图纸里"显示线宽"总开关的初值（-1 则前端按关处理）
    try:
        out["lwd"] = 1 if db.lineWeightDisplay() else 0
    except Exception:
        pass

    defs = {}       # 块名 → 定义指纹
    sub_refs = {}   # 块名 → ObjectId（补读嵌套块定义用，不进 JSON）
    style_ids = {}  # 样式 id 字符串 → 字体文件名

    with P.Db.PyDbTrans() as tr:
        # 图层色表：ByLayer(256) 的实体最终用这个色（底图灰通常就落在图层色上）
        try:
            for lid in db.objectIds("AcDbLayerTableRecord"):
                try:
                    lr = tr.openObject(lid, 0)
                    nm = str(lr.getName())
                    out["layer_col"][nm] = _layer_color(lr)
                    try:
                        out["layer_lw"][nm] = int(lr.lineWeight())
                    except Exception:
                        out["layer_lw"][nm] = -3
                except Exception:
                    pass
        except Exception as ex:
            out["err"].append("layerCol:" + str(ex)[:40])
        sp = tr.openObject(db.currentSpaceId(), 0)
        ids = sp.entityIds()
        out["counts"]["total"] = len(ids)

        for eid in ids:
            try:
                o = tr.openObject(eid, 0)
                # 同块内一样：类名优先，否则退回 dxfName
                short = _dxftype(o, eid)
            except Exception as ex:
                out["err"].append("open:" + str(ex)[:40])
                continue
            out["counts"][short] = out["counts"].get(short, 0) + 1
            try:
                ln = str(o.layer())
            except Exception:
                ln = "0"
            out["layers"][ln] = out["layers"].get(ln, 0) + 1
            # 颜色：ACI 索引色。256=ByLayer、0=ByBlock，其余是实体自有色。
            # 底图灰（8/9/250~255）和电气彩线靠这个区分，是剔除底图的唯一可靠依据。
            ci = 256
            try:
                ci = int(o.colorIndex())
            except Exception:
                try:
                    ci = int(o.color())
                except Exception:
                    ci = 256
            # ByLayer(256)：真实显示色是该图层的颜色。图层表遍历不来
            # （objectIds("AcDbLayerTableRecord") 恒 0），就从实体自己的 layerId 反查，
            # 走到哪个图层查哪个，一次缓存。
            if ci >= 256 and ln not in out["layer_col"]:
                try:
                    lr = tr.openObject(o.layerId(), 0)
                    out["layer_col"][ln] = _layer_color(lr)
                except Exception:
                    pass

            # 线宽：AcDb::LineWeight 枚举（-3 Default/-2 ByBlock/-1 ByLayer，0..211 = 0.00~2.11mm）。
            # 前端"线宽开关"开着时才用它换算像素宽度。
            lw = -1
            try:
                lw = int(o.lineWeight())
            except Exception:
                pass
            # 图层表 objectIds 恒 0（见上方 layer_col 注释），ByLayer 只能顺着实体反查：
            # 走到哪个图层就记哪个图层的线宽，后续所有同层实体直接命中缓存。
            if lw == -1 and ln not in out["layer_lw"]:
                try:
                    lr = tr.openObject(o.layerId(), 0)
                    out["layer_lw"][ln] = int(lr.lineWeight())
                except Exception:
                    pass

            try:
                cls = MAP.get(short, short)

                if cls == "AcDbLine":
                    a = _pt(o, ("startPoint", "getStartPoint"))
                    b = _pt(o, ("endPoint", "getEndPoint"))
                    if a and b:
                        out["lines"].append([round(a[0], 2), round(a[1], 2),
                                             round(b[0], 2), round(b[1], 2), ln, ci, lw])

                elif cls in ("AcDbPolyline", "AcDb2dPolyline"):
                    pts = []
                    n = _num(o, ("numVerts",), 0) or 0
                    for i in range(int(n)):
                        q = None
                        for fn in ("getPointAt", "getPointAtParam"):
                            if hasattr(o, fn):
                                try:
                                    q = getattr(o, fn)(i)
                                    break
                                except Exception:
                                    pass
                        if q is not None:
                            pts.append([round(q.x, 2), round(q.y, 2)])
                    if len(pts) >= 2:
                        cl = bool(_num(o, ("isClosed",), False))
                        out["plines"].append({"p": pts, "c": cl, "l": ln, "ci": ci, "lw": lw})

                elif cls == "AcDbCircle":
                    c0 = _pt(o, ("center", "getCenter"))
                    r = float(_num(o, ("radius",), 0.0) or 0.0)
                    if c0:
                        out["circles"].append([round(c0[0], 2), round(c0[1], 2), round(r, 2), ln, ci, lw])

                elif cls == "AcDbArc":
                    c0 = _pt(o, ("center", "getCenter"))
                    r = float(_num(o, ("radius",), 0.0) or 0.0)
                    if c0:
                        out["arcs"].append({"c": [round(c0[0], 2), round(c0[1], 2)], "r": round(r, 2),
                                            "a0": round(float(_num(o, ("startAngle",), 0.0)), 4),
                                            "a1": round(float(_num(o, ("endAngle",), 0.0)), 4), "l": ln,
                                            "ci": ci, "lw": lw})

                elif cls in ("AcDbText", "AcDbMText"):
                    s = _clean(_str(o, ("textString", "contents", "text")))
                    if not s.strip():
                        continue
                    ha = int(_num(o, ("horizontalMode",), 0) or 0)
                    va = int(_num(o, ("verticalMode",), 0) or 0)
                    ap = _pt(o, ("alignmentPoint", "getAlignmentPoint"))
                    pos = _pt(o, ("position", "getPosition", "location"))
                    if (ha or va) and ap and (abs(ap[0]) + abs(ap[1]) > 1e-6):
                        pos = ap
                    if cls == "AcDbMText":
                        at = int(_num(o, ("attachment",), 1) or 1)
                        ha, va = {1: (0, 3), 2: (1, 3), 3: (2, 3), 4: (0, 2), 5: (1, 2),
                                  6: (2, 2), 7: (0, 0), 8: (1, 0), 9: (2, 0)}.get(at, (0, 3))
                    if not pos:
                        continue
                    # 文字样式 → 字体文件名（缓存：样式 id 有限）
                    st = ""
                    try:
                        sid = o.textStyle() if hasattr(o, "textStyle") else None
                        key = str(sid)
                        if key not in style_ids:
                            rec = tr.openObject(sid, 0)
                            style_ids[key] = _str(rec, ("fileName", "bigFontFileName"))
                        st = style_ids[key]
                    except Exception:
                        pass
                    out["texts"].append({
                        "x": round(pos[0], 2), "y": round(pos[1], 2), "t": s,
                        "h": round(float(_num(o, ("height", "textHeight"), 250.0) or 250.0), 2),
                        "r": round(float(_num(o, ("rotation",), 0.0) or 0.0), 4),
                        "ha": ha, "va": va, "wf": round(float(_num(o, ("widthFactor",), 1.0) or 1.0), 3),
                        "ci": ci,
                        "l": ln, "st": st.split("/")[-1] if st else "",
                        "mt": 1 if cls == "AcDbMText" else 0})

                elif cls in ("AcDbBlockReference", "AcDbMInsertBlock"):
                    name = "?"
                    btr = None
                    try:
                        btr = tr.openObject(o.blockTableRecordId(), 0)
                        name = str(btr.getName())
                    except Exception:
                        pass
                    pos = _pt(o, ("position", "location"))
                    sc = 1.0
                    scy = 1.0
                    try:
                        sf = o.scaleFactors()
                        sc = round(float(sf[0]), 3)
                        scy = round(float(sf[1]), 3)
                    except Exception:
                        try:
                            tm = o.blockTransform().toNumpy()
                            sc = round(float(tm[0][0]), 3)
                            scy = round(float(tm[1][1]), 3)
                        except Exception:
                            pass
                    rec = {"n": name,
                           "p": [round(pos[0], 1), round(pos[1], 1)] if pos else None,
                           "b": _bbox(o), "l": ln,
                           "r": round(float(_num(o, ("rotation",), 0.0) or 0.0), 3),
                           "s": sc, "sy": scy, "ci": ci}
                    try:
                        rec["h"] = str(o.handle()).upper()
                    except Exception:
                        pass
                    # 块参照的真实 2D 仿射：rotation()/scaleFactors() 拼出来的矩阵
                    # 遇到镜像块（sx=-1）会和 CAD 实际变换对不上，箱子就被画歪。
                    # 这里直接导出 blockTransform 的 2x2 + 插入点，网页端原样套用。
                    try:
                        tm = o.blockTransform().toNumpy()
                        tx, ty = (pos if pos else (float(tm[0][3]), float(tm[1][3])))
                        rec["m"] = [round(float(tm[0][0]), 6), round(float(tm[0][1]), 6),
                                    round(float(tm[1][0]), 6), round(float(tm[1][1]), 6),
                                    round(float(tx), 3), round(float(ty), 3)]
                    except Exception:
                        pass
                    # 块属性（ATTRIB）：设备编号/型号/回路名常放这里。
                    # attributeIds() 给的是旧式 IntDbId，必须用块参照自己的
                    # openAttribute 打开；早前拿它去 tr.openObject 必然失败。
                    try:
                        ids = o.attributeIds() if hasattr(o, "attributeIds") else []
                        attrs, atx = {}, []
                        for aid in list(ids)[:64]:
                            a = None
                            try:
                                # mode 必须是 Enum.OpenMode 枚举，传 int 会被 pybind 类型检查拒绝
                                a = o.openAttribute(aid, P.Enum.OpenMode.kForRead, False)
                            except Exception:
                                a = None
                            if a is None:
                                continue
                            try:
                                tag = _str(a, ("tag", "tagString"))
                                val = _clean(_str(a, ("textString", "value")))
                                if tag:
                                    attrs[tag] = val
                                ap = _pt(a, ("position", "alignmentPoint"))
                                ah = float(_num(a, ("height",), 0.0) or 0.0)
                                ar = float(_num(a, ("rotation",), 0.0) or 0.0)
                                aha = int(_num(a, ("horizontalMode",), 0) or 0)
                                ainv = bool(_num(a, ("isInvisible",), False))
                                if ap and val.strip() and ah > 0 and not ainv:
                                    atx.append([round(ap[0], 2), round(ap[1], 2), val,
                                                round(ah, 2), round(ar, 4), aha, ci])
                            except Exception:
                                continue
                        if attrs:
                            rec["at"] = attrs
                        if atx:
                            rec["atx"] = atx
                    except Exception as ex:
                        rec["at_probe"] = ["EXC:%s" % str(ex)[:40]]
                    out["blocks"].append(rec)
                    # 块定义只读一次：类型统计 + 文本 + **几何**（几何必须在块坐标系下
                    # 单独导出，否则平面图的设备符号只剩一个 bbox 空框）
                    if btr is not None and name not in defs:
                        try:
                            bids = btr.entityIds()
                            types, texts = {}, []
                            gl, gp, gc, ga, gt, gi = [], [], [], [], [], []
                            for beid in bids:
                                bo = tr.openObject(beid, 0)
                                bt = bo.dxfName()
                                types[bt] = types.get(bt, 0) + 1
                                # 块内图元自己的色号：0=ByBlock（要取**最顶层** blockref 的色），
                                # 256=ByLayer（取所在图层色），其余为显式色。
                                # 不导出这三档就只能在渲染时整块刷一个色，显式色会被吃掉。
                                bci = 256
                                try:
                                    bci = int(bo.colorIndex())
                                except Exception:
                                    pass
                                bly = "0"
                                try:
                                    bly = str(bo.layer())
                                except Exception:
                                    pass
                                try:
                                    bcls = MAP.get(bt, bt)
                                    if bcls == "AcDbLine" and len(gl) < 90000:
                                        a = _pt(bo, ("startPoint", "getStartPoint"))
                                        b2 = _pt(bo, ("endPoint", "getEndPoint"))
                                        if a and b2:
                                            gl.append([round(a[0], 1), round(a[1], 1),
                                                       round(b2[0], 1), round(b2[1], 1),
                                                       bci, bly])
                                    elif bcls in ("AcDbPolyline", "AcDb2dPolyline") and len(gp) < 24000:
                                        pts = []
                                        for i in range(int(_num(bo, ("numVerts",), 0) or 0)):
                                            q = None
                                            for fn in ("getPointAt", "getPointAtParam"):
                                                if hasattr(bo, fn):
                                                    try:
                                                        q = getattr(bo, fn)(i)
                                                        break
                                                    except Exception:
                                                        pass
                                            if q is not None:
                                                pts.append([round(q.x, 1), round(q.y, 1)])
                                        if len(pts) >= 2:
                                            gp.append({"p": pts,
                                                       "c": bool(_num(bo, ("isClosed",), False)),
                                                       "ci": bci, "ly": bly})
                                    elif bcls in ("AcDbBlockReference", "AcDbMInsertBlock") and len(gi) < 9000:
                                        # 嵌套块：底图 1269 个墙/门/轴号圈都在这一层，
                                        # 不记录就等于底图缺一大半
                                        sn = "?"
                                        try:
                                            sn = str(tr.openObject(bo.blockTableRecordId(), 0).getName())
                                        except Exception:
                                            pass
                                        p = _pt(bo, ("position", "location"))
                                        s1, s2 = 1.0, 1.0
                                        try:
                                            sf2 = bo.scaleFactors()
                                            s1 = round(float(sf2[0]), 3)
                                            s2 = round(float(sf2[1]), 3)
                                        except Exception:
                                            pass
                                        if p:
                                            if sn not in defs:
                                                # 当场递归读子块（见 _scan_bdef 注释：
                                                # ObjectId 延后再打开会失效）
                                                try:
                                                    sb0 = tr.openObject(bo.blockTableRecordId(), 0)
                                                    defs[sn] = (_scan_bdef(tr, sb0, defs, 4)
                                                                if sb0 is not None
                                                                else {"err": "openObject None"})
                                                except Exception as ex:
                                                    defs[sn] = {"err": str(ex)[:60]}
                                            sg = {"n": sn,
                                                  "p": [round(p[0], 1), round(p[1], 1)],
                                                  "r": round(float(_num(bo, ("rotation",), 0.0) or 0.0), 4),
                                                  "s": s1, "sy": s2,
                                                  "ci": bci, "ly": bly}
                                            try:
                                                tm2 = bo.blockTransform().toNumpy()
                                                sg["m"] = [round(float(tm2[0][0]), 6),
                                                           round(float(tm2[0][1]), 6),
                                                           round(float(tm2[1][0]), 6),
                                                           round(float(tm2[1][1]), 6),
                                                           round(float(p[0]), 3),
                                                           round(float(p[1]), 3)]
                                            except Exception:
                                                pass
                                            gi.append(sg)
                                    elif bcls == "AcDbCircle" and len(gc) < 9000:
                                        c0 = _pt(bo, ("center", "getCenter"))
                                        r = float(_num(bo, ("radius",), 0.0) or 0.0)
                                        if c0:
                                            gc.append([round(c0[0], 1), round(c0[1], 1), round(r, 1),
                                                       bci, bly])
                                    elif bcls == "AcDbArc" and len(ga) < 9000:
                                        c0 = _pt(bo, ("center", "getCenter"))
                                        r = float(_num(bo, ("radius",), 0.0) or 0.0)
                                        if c0:
                                            ga.append([round(c0[0], 1), round(c0[1], 1), round(r, 1),
                                                       round(float(_num(bo, ("startAngle",), 0.0)), 3),
                                                       round(float(_num(bo, ("endAngle",), 0.0)), 3),
                                                       bci, bly])
                                    elif bcls in ("AcDbText", "AcDbMText") and len(gt) < 4000:
                                        s = _clean(_str(bo, ("textString", "contents", "text")))
                                        pos = _pt(bo, ("position", "getPosition", "location"))
                                        if s.strip() and pos:
                                            gt.append({"x": round(pos[0], 1), "y": round(pos[1], 1),
                                                       "t": s[:24],
                                                       "h": round(float(_num(bo, ("height", "textHeight"), 250.0) or 250.0), 1),
                                                       "r": round(float(_num(bo, ("rotation",), 0.0) or 0.0), 3),
                                                       "wf": round(float(_num(bo, ("widthFactor",), 1.0) or 1.0), 2),
                                                       "ha": int(_num(bo, ("horizontalMode",), 0) or 0),
                                                       "ci": bci, "ly": bly})
                                except Exception:
                                    pass
                                if bt in ("TEXT", "MTEXT", "ATTRIB") and len(texts) < _TEXTMAX:
                                    txt = _clean(_str(bo, ("textString", "contents")))
                                    if txt.strip():
                                        texts.append(txt.strip()[:48])
                            defs[name] = {"n": len(bids), "types": types, "texts": texts,
                                          "b": _bbox(btr),
                                          "geom": {"lines": gl, "plines": gp, "circles": gc,
                                                   "arcs": ga, "texts": gt, "ins": gi}}
                        except Exception as ex:
                            defs[name] = {"err": str(ex)[:60]}

                elif cls == "AcDbTable":
                    rows = int(_num(o, ("numRows",), 0) or 0)
                    cols = int(_num(o, ("numCols", "numColumns"), 0) or 0)
                    if not cols:
                        # 探测列数：第一行一直读到空/异常
                        cols = 0
                        for c in range(64):
                            try:
                                t = _str(o, ("textString",))  # 占位，实际下面按 (r,c) 调
                                cols = c
                                break
                            except Exception:
                                break
                        cols = 0
                        for c in range(64):
                            try:
                                s = o.textString(0, c)
                                if s is None:
                                    break
                                cols = c + 1
                            except Exception:
                                break
                    cells = []
                    for r_ in range(min(rows, 200)):
                        row = []
                        for c_ in range(min(cols, 60)):
                            try:
                                row.append(_clean(o.textString(r_, c_))[:80])
                            except Exception:
                                row.append("")
                        cells.append(row)
                    out["tables"].append({"rows": rows, "cols": cols, "cells": cells,
                                          "b": _bbox(o), "l": ln})

                elif cls == "AcDbPolyline" and short == "POLYLINE":
                    pass
            except Exception as ex:
                out["err"].append("%s:%s" % (short, str(ex)[:50]))

        # 属性块文字必须在事务内采集：形参 tr 一旦离开 with 就失效，openObject 返回 None。
        try:
            _walk_attr(tr, tr.openObject(db.currentSpaceId(), 0), out["attrib"], 0)
        except Exception as ex:
            out["err"].append("attrib:" + str(ex)[:60])

    # ---- 补读块定义内部的嵌套块：$TwtSys$ 图签表格、*U 匿名块只在子块位置出现，
    # 主循环按顶层 INSERT 读定义时永远读不到，A1 图框里大片内容会整块丢失。
    depth = 0
    while depth < 8:
        pend = {sn: bid for sn, bid in sub_refs.items() if sn not in defs}
        if not pend:
            break
        depth += 1
        for sn, bid in pend.items():
            if sn in defs:
                continue
            try:
                defs[sn] = _scan_bdef(tr, tr.openObject(bid, 0), sub_refs)
                out["counts"]["block_defs_patch"] = out["counts"].get("block_defs_patch", 0) + 1
            except Exception as ex:
                defs[sn] = {"err": str(ex)[:60]}
    out["block_defs"] = defs
    out["err"] = out["err"] + _DBG
    out["styles"] = {k: v for k, v in style_ids.items() if v}

    # 图框候选：闭合矩形，面积占图面 ≥ 60%，宽高比接近 A 系（1.35~1.45）
    big = []
    for p in out["plines"]:
        if not p["c"]:
            continue
        xs = [q[0] for q in p["p"]]
        ys = [q[1] for q in p["p"]]
        w, h = max(xs) - min(xs), max(ys) - min(ys)
        if w > 20000 and h > 15000 and w / h < 2.2 and h / w < 2.2:
            big.append([round(min(xs)), round(min(ys)), round(max(xs)), round(max(ys))])
    # 去重（同尺寸多圈线只留一个）
    uniq = []
    for b in sorted(big, key=lambda b: -((b[2] - b[0]) * (b[3] - b[1]))):
        if any(abs(b[0] - u[0]) < 300 and abs(b[1] - u[1]) < 300 and
               abs(b[2] - u[2]) < 300 and abs(b[3] - u[3]) < 300 for u in uniq):
            continue
        uniq.append(b)
    out["frame_cands"] = uniq[:120]
    if uniq:
        out["frame"] = [min(u[0] for u in uniq), min(u[1] for u in uniq),
                        max(u[2] for u in uniq), max(u[3] for u in uniq)]

    out["counts"]["lines"] = len(out["lines"])
    out["counts"]["plines"] = len(out["plines"])
    out["counts"]["texts"] = len(out["texts"])
    out["counts"]["blocks"] = len(out["blocks"])
    out["counts"]["tables"] = len(out["tables"])
    out["counts"]["block_defs"] = len(defs)
    out["counts"]["attrib"] = len([a for a in out["attrib"] if a and a[0] != "E1"])

    with open(OUTPUT, "w", encoding="utf-8") as f:
        json.dump(out, f, ensure_ascii=False)

    result = "scan ok: %d ents, %d blockdefs, %d tables" % (
        out["counts"]["total"], len(defs), len(out["tables"]))


run()
