# -*- coding: utf-8 -*-
"""把 drawTexts 的对齐/基线修正打补丁到已导出的 scene.html（免重扫）。"""
import io
import sys

OLD1 = """    let lastFont = '', lastCol = '', lastAlign = -1;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.textBaseline = 'alphabetic';"""
NEW1 = """    let lastFont = '', lastCol = '', lastAlign = -1, lastBase = '';
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.textBaseline = 'alphabetic';
    lastBase = 'alphabetic';"""

OLD2 = """      const al = t[4] === 1 ? 1 : (t[4] === 2 ? 2 : 0);
      if (al !== lastAlign) {
        ctx.textAlign = al === 1 ? 'center' : (al === 2 ? 'right' : 'left');
        lastAlign = al;
      }"""
NEW2 = """      // ha: 0=左 1=中 2=右 3=对齐 4=正中 5=布满 —— 3/4/5 都按居中处理（轴号圆内数字就是这个）
      const al = t[4] === 2 ? 2 : ((t[4] === 1 || t[4] === 3 || t[4] === 4 || t[4] === 5) ? 1 : 0);
      if (al !== lastAlign) {
        ctx.textAlign = al === 1 ? 'center' : (al === 2 ? 'right' : 'left');
        lastAlign = al;
      }
      // va: 0=基线 1=底 2=中 3=顶 —— 真正的中对齐要让字形中心落在锚点
      const vb = t[8] === 2 ? 'middle' : (t[8] === 3 ? 'top' : (t[8] === 1 ? 'bottom' : 'alphabetic'));
      if (vb !== lastBase) { ctx.textBaseline = vb; lastBase = vb; }"""

OLD3 = "      lastFont = ''; lastCol = ''; lastAlign = -1;   // restore 把 canvas 状态弹回去了"
NEW3 = "      lastFont = ''; lastCol = ''; lastAlign = -1; lastBase = '';   // restore 把 canvas 状态弹回去了"

path = sys.argv[1]
s = io.open(path, encoding="utf-8", errors="surrogateescape").read()
for i, (o, n) in enumerate(((OLD1, NEW1), (OLD2, NEW2), (OLD3, NEW3)), 1):
    c = s.count(o)
    print("patch%d hits=%d" % (i, c))
    if c:
        s = s.replace(o, n)
io.open(path, "w", encoding="utf-8", errors="surrogateescape").write(s)
print("done", path)
