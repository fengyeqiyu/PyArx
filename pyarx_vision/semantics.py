# -*- utf-8 -*-
"""pyarx_vision.semantics —— 文字语义分类 + 一对一标注归属

分类靠"正则词典"，归属靠"同列 + 上下邻域 + 贪心去重"：
一条标注只能属于一个单元，杜绝两个单元抢同一段文字（这是纯最近邻做不到的）。
"""
import re

# ---------------------------------------------------------------- 分类
PATTERNS = [
    ("cap", re.compile(r"^\d+A$")),
    ("ct", re.compile(r"^\d+/5$")),
    ("cable", re.compile(r"(YJ|KVV|BLV|VV)")),
    ("meter", re.compile(r"电表|Wh")),
    ("spare", re.compile(r"备用")),
    ("bus", re.compile(r"母线槽|母线")),
    ("station", re.compile(r"变电站|配电室|开闭所")),
    ("load", re.compile(r"(Pe\s*=|Kx\s*=|Ijs\s*=|Cos|cos)")),
    ("tag", re.compile(r"^[0-9A-Za-z]{2,}-[0-9A-Za-z\-]+")),
    ("note", re.compile(r"(注：|整定|回路编号|说明)")),
]


def classify(text):
    """单条文字 → 语义类别（cap/ct/cable/meter/spare/bus/station/load/tag/note/other）"""
    s = (text or "").strip()
    for kind, pat in PATTERNS:
        if pat.search(s):
            return kind
    return "other"


def classify_all(es):
    """整张图的文字按类别分桶"""
    out = {}
    for t in es["texts"]:
        out.setdefault(classify(t["text"]), []).append(t)
    return out


# ---------------------------------------------------------------- 归属
DEFAULT_SPEC = [
    # kind,   y 半径,  x 半径, 是否必须在单元上方
    ("cap",   3200,    1100,  False),
    ("ct",    9000,    1100,  False),
    ("meter", 9000,    1100,  False),
    ("spare", 14000,   1100,  False),
    ("cable", 14000,   1100,  True),
    ("tag",   20000,   2600,  True),
]


def assign(loops, pools, spec=None, size_key="cap"):
    """一对一贪心分配：距离优先、每个 loop 每类只收一条、每条文字只用一次

    loops: [{"x":, "y":, ...}]，就地写入 cap/tag/cable/ct/meter/spare 与 conf
    pools: classify_all 的结果
    返回每类的 (命中数, 候选数) 统计
    """
    spec = spec or DEFAULT_SPEC
    stats = {}
    # 预置字段，调用方不必猜哪些键可能不存在
    for lo in loops:
        for k in ("cap", "tag", "cable", "ct", "spare"):
            lo.setdefault(k, "")
        lo.setdefault("meter", False)
        lo.setdefault("conf", {})
    for kind, ylim, dxlim, above_only in spec:
        cands = [t for t in pools.get(kind, [])]
        if kind == "cap":
            # 母线额定容量的字高明显大于回路容量，先按字高中位数剔除
            if cands:
                hs = sorted(t["height"] for t in cands)
                mid = hs[len(hs) // 2]
                cands = [t for t in cands if t["height"] <= mid * 1.6]
        pairs = []
        for ti, t in enumerate(cands):
            tx, ty = t["pos"]
            for li, lo in enumerate(loops):
                dx = abs(tx - lo["x"])
                dy = ty - lo["y"] if above_only else abs(ty - lo["y"])
                if dx > dxlim or dy > ylim or dy < -200:
                    continue
                pairs.append((dx + abs(dy) * 0.2, li, ti, t))
        pairs.sort(key=lambda p: p[0])
        used_l, used_t = set(), set()
        for dist, li, ti, t in pairs:
            if li in used_l or ti in used_t:
                continue
            used_l.add(li)
            used_t.add(ti)
            lo = loops[li]
            s = t["text"].strip()
            if kind == "cap":
                lo["cap"] = s.rstrip("A")
            elif kind == "meter":
                lo["meter"] = True
            else:
                lo[kind] = s
            lo.setdefault("conf", {})[kind] = round(
                max(0.0, 1.0 - dist / (ylim + dxlim)), 3)
        stats[kind] = (len(used_t), len(cands))
    return stats


def bus_info(buses, pools, pad=500.0, ylim=6000.0):
    """把 spec/station/load 文字挂到母线段（同一段可挂多条负荷文字）"""
    for b in buses:
        b["info"] = {}
        b["cap"] = ""
    for kind in ("bus", "station", "load"):
        for t in pools.get(kind, []):
            tx, ty = t["pos"]
            best, bd = None, None
            for b in buses:
                if not (b["x0"] - pad <= tx <= b["x1"] + pad):
                    continue
                dy = abs(ty - b["top"])
                if dy > ylim:
                    continue
                if bd is None or dy < bd:
                    best, bd = b, dy
            if best is None:
                continue
            best["info"].setdefault(kind, [])
            if t["text"].strip() not in best["info"][kind]:
                best["info"][kind].append(t["text"].strip())
    # 额定容量：大字高的 cap
    for t in pools.get("cap", []):
        if t["height"] < 300:
            continue
        tx, ty = t["pos"]
        best, bd = None, None
        for b in buses:
            if b["x0"] - pad <= tx <= b["x1"] + pad and abs(ty - b["top"]) < 3000:
                dy = abs(ty - b["top"])
                if bd is None or dy < bd:
                    best, bd = b, dy
        if best is not None and not best["cap"]:
            best["cap"] = t["text"].strip()

    for b in buses:
        info = b["info"]
        b["spec"] = (info.get("spec") or info.get("bus") or [""])[0]
        b["station"] = (info.get("station") or [""])[0]
        load = " ".join(info.get("load", []))
        def grab(pat):
            m = re.search(pat, load)
            return m.group(0) if m else ""
        b["pe"] = grab(r"Pe\s*=\s*[^\s，,]+")
        b["cos"] = grab(r"(Cos|cos)[^0-9]*[=\s][^\s，,]*")
        b["kx"] = " ".join(x for x in (grab(r"Kx\s*=\s*[^\s，,]+"),
                                      grab(r"Ijs\s*=\s*[^\s，,]+")) if x)
    return buses
