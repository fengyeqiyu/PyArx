# -*- coding: utf-8 -*-
"""看图层：名字、颜色、默认可见性，以及顶层线按图层的分布（找那片密集网格来自哪层）。"""
import json, collections

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

ly = D["ly"]; lyh = D.get("lyh"); lyc = D.get("lyc") or D.get("lycol")
print("图层数", len(ly))
if isinstance(lyh, list):
    hidden = [i for i, v in enumerate(lyh) if v]
    print("默认隐藏:", [(i, ly[i]) for i in hidden])

ln = D["ln"]
cnt = collections.Counter()
for i in range(0, len(ln), 6):
    if i + 5 < len(ln):
        cnt[ln[i + 5]] += 1
print("\n顶层线按图层（前 12）：")
for k, v in cnt.most_common(12):
    name = ly[k] if isinstance(k, int) and 0 <= k < len(ly) else str(k)
    print("  %-28s %6d 条  color=%s" % (name[:28], v, (lyc[k] if isinstance(lyc, list) and 0 <= k < len(lyc) else "?")))
