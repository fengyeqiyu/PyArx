# -*- coding: utf-8 -*-
"""把 drawTexts 的 ha/va 版式修正打进已导出的 scene.html。幂等：已打过就跳过。"""
import io
import sys

# 1) 去掉 lastBase 机制
OLD1 = """    let lastFont = '', lastCol = '', lastAlign = -1, lastBase = '';
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.textBaseline = 'alphabetic';
    lastBase = 'alphabetic';"""
NEW1 = """    let lastFont = '', lastCol = '', lastAlign = -1;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.textBaseline = 'alphabetic';"""

OLD3 = "      lastFont = ''; lastCol = ''; lastAlign = -1; lastBase = '';   // restore 把 canvas 状态弹回去了"
NEW3 = "      lastFont = ''; lastCol = ''; lastAlign = -1;   // restore 把 canvas 状态弹回去了"

# 2) 对齐 + 垂直偏移（替换掉上一版 ha-only 与 va-middle 两种写法）
OLD2A = """      const al = t[4] === 1 ? 1 : (t[4] === 2 ? 2 : 0);
      if (al !== lastAlign) {
        ctx.textAlign = al === 1 ? 'center' : (al === 2 ? 'right' : 'left');
        lastAlign = al;
      }"""

NEW2 = """      // ha: 0=\u5de6 1=\u4e2d 2=\u53f3 3=\u5bf9\u9f50 4=\u6b63\u4e2d 5=\u5e03\u6ee1 \u2014\u2014 3/4/5 \u90fd\u6309\u5c45\u4e2d\u5904\u7406\uff08\u8f74\u53f7\u5708\u5185\u6570\u5b57\u5c31\u662f\u8fd9\u4e2a\uff09
      const al = t[4] === 2 ? 2 : ((t[4] === 1 || t[4] === 3 || t[4] === 4 || t[4] === 5) ? 1 : 0);
      if (al !== lastAlign) {
        ctx.textAlign = al === 1 ? 'center' : (al === 2 ? 'right' : 'left');
        lastAlign = al;
      }
      // va: 0=\u57fa\u7ebf 1=\u5e95 2=\u4e2d 3=\u9876\u3002canvas \u7684 'middle'/'top' \u662f\u6309 em \u6846\u7b97\u7684\uff0c
      // \u6570\u5b57\u843d\u5728 em \u6846\u4e0a\u534a\u90e8\uff0c\u7528\u5b83\u4f1a\u8ba9\u8f74\u53f7\u5728\u5708\u91cc\u504f\u4e0b\uff1b\u8fd9\u91cc\u7edf\u4e00\u7528\u57fa\u7ebf + \u5b57\u5f62\u771f\u5b9e
      // \u5305\u56f4\u76d2\uff08actualBoundingBoxAscent/Descent\uff09\u6362\u7b97\u504f\u79fb\uff0c\u89c6\u89c9\u624d\u771f\u6b63\u5c45\u4e2d\u3002
      // ha>=3 \u4e14 va=0 \u7684\u77ed\u6807\u6ce8\uff08\u8f74\u53f7 1\u202628\u3001\u56de\u8def\u53f7\uff09\u6309\u5782\u76f4\u5c45\u4e2d\u5904\u7406\u3002
      const _short = String(t[6]).length <= 3;
      const vmode = (t[8] === 2 || (t[4] >= 3 && !t[8] && _short)) ? 2
        : (t[8] === 3 ? 3 : (t[8] === 1 ? 1 : 0));
      let dyv = 0;
      if (vmode) {
        const m2 = ctx.measureText(t[6]);
        const a2 = m2.actualBoundingBoxAscent || 0, d2 = m2.actualBoundingBoxDescent || 0;
        dyv = vmode === 2 ? (a2 - d2) / 2 : (vmode === 3 ? a2 : -d2);
      }"""

OLD2B = """      // va: 0=\u57fa\u7ebf 1=\u5e95 2=\u4e2d 3=\u9876 \u2014\u2014 \u771f\u6b63\u7684\u4e2d\u5bf9\u9f50\u8981\u8ba9\u5b57\u5f62\u4e2d\u5fc3\u843d\u5728\u951a\u70b9\u3002
      // ha>=3\uff08\u6b63\u4e2d/\u5e03\u6ee1/\u5bf9\u9f50\uff09\u4e14\u6ca1\u7ed9 va \u7684\u77ed\u6807\u6ce8\uff08\u8f74\u53f7 1\u202628\u3001\u56de\u8def\u53f7\uff09\u4e00\u5f8b\u6309\u5782\u76f4\u5c45\u4e2d\uff0c
      // \u8fd9\u6837\u5b83\u843d\u5728\u8f74\u53f7\u5708\u91cc\u5c31\u662f\u51e0\u4f55\u5c45\u4e2d\uff0c\u548c CAD \u663e\u793a\u4e00\u81f4\u3002
      const _short = String(t[6]).length <= 3;
      const vb = (t[8] === 2 || (t[4] >= 3 && !t[8] && _short))
        ? 'middle' : (t[8] === 3 ? 'top' : (t[8] === 1 ? 'bottom' : 'alphabetic'));
      if (vb !== lastBase) { ctx.textBaseline = vb; lastBase = vb; }
      // ha: 0=\u5de6 1=\u4e2d 2=\u53f3 3=\u5bf9\u9f50 4=\u6b63\u4e2d 5=\u5e03\u6ee1 \u2014\u2014 3/4/5 \u90fd\u6309\u5c45\u4e2d\u5904\u7406\uff08\u8f74\u53f7\u5708\u5185\u6570\u5b57\u5c31\u662f\u8fd9\u4e2a\uff09
      const al = t[4] === 2 ? 2 : ((t[4] === 1 || t[4] === 3 || t[4] === 4 || t[4] === 5) ? 1 : 0);
      if (al !== lastAlign) {
        ctx.textAlign = al === 1 ? 'center' : (al === 2 ? 'right' : 'left');
        lastAlign = al;
      }"""

OLD4 = """      if (!a) {
        ctx.fillText(t[6], wp.x, wp.y);
        continue;
      }"""
NEW4 = """      if (!a) {
        ctx.fillText(t[6], wp.x, wp.y + dyv);
        continue;
      }"""

OLD5 = """      ctx.rotate(Math.atan2(dir.y - wp.y, dir.x - wp.x));
      ctx.fillText(t[6], 0, 0);"""
NEW5 = """      ctx.rotate(Math.atan2(dir.y - wp.y, dir.x - wp.x));
      ctx.fillText(t[6], 0, dyv);"""

path = sys.argv[1]
s = io.open(path, encoding="utf-8", errors="surrogateescape").read()
pairs = [(OLD1, NEW1), (OLD3, NEW3), (OLD2A, NEW2), (OLD2B, NEW2), (OLD4, NEW4), (OLD5, NEW5)]
for i, (o, n) in enumerate(pairs, 1):
    c = s.count(o)
    print("p%d hits=%d" % (i, c))
    if c:
        s = s.replace(o, n)
io.open(path, "w", encoding="utf-8", errors="surrogateescape").write(s)
print("done")
