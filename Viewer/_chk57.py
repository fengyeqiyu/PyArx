# -*- coding: utf-8 -*-
"""查块定义文字(tx)与块参照属性(atx)在同一位置重复渲染的对数——ATTDEF 与 ATTRIB 叠字。"""
import json, collections

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

blk = D["blk"]
ins = D["ins"]

n_def_tx = sum(len(d.get("tx") or []) for d in blk.values())
n_blocks_with_tx = sum(1 for d in blk.values() if d.get("tx"))
n_ins = sum(1 for e in ins if e.get("atx"))
print("块定义文字条目 %d，含文字的块定义 %d 个 / %d" % (n_def_tx, n_blocks_with_tx, len(blk)))
print("带属性文字的块参照 %d / %d" % (n_ins, len(ins)))

dup = 0
shown = 0
for e in ins:
    atx = e.get("atx")
    if not atx:
        continue
    d = blk.get(e["k"]) or {}
    tx = d.get("tx") or []
    if not tx:
        continue
    # 块内文字在块坐标系里比较
    for t in tx:
        for at in atx:
            if abs(t[0] - at[0]) <= 6 and abs(t[1] - at[1]) <= 6:
                dup += 1
                if shown < 8:
                    print("   重复 %-30s 块内'%s' vs 属性'%s' @(%.0f,%.0f)"
                          % (e["k"][:30], str(t[6])[:12], str(at[6])[:12], t[0], t[1]))
                    shown += 1
                break

print("\n同位置重复（块内文字 vs 属性值）合计 %d 对" % dup)
