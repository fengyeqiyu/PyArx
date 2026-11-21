# -*- coding: utf-8 -*-
"""两条入口等价性：guiload(文件) 与 guiload_text(字符串) 落到同一批控件、同一尺寸。

这是"像 HTML 那样建界面"的底气：字符串入口不是另写一套，而是同一引擎。
"""
import os
import sys

from _kit import M, check, finish, ROOT

SRC = os.path.join(ROOT, 'plugins', 'side_panel.xml')
SAMPLES = ['BtnApply', 'BtnLoad', 'Table']


def sizes(win, names, keys):
    out = {}
    for k in keys:
        cid = names.get(k)
        out[k] = None if cid is None else tuple(win.get_size(cid))
    return out


def main():
    with open(SRC, 'r', encoding='utf-8') as f:
        text = f.read()

    n1, n2 = {}, {}
    w1 = M.Window('t-parity-file', 700, 520)
    M.guiload(SRC, target=w1, context=globals(), names=n1)
    w1.show()
    M.pump(120)

    w2 = M.Window('t-parity-text', 700, 520)
    M.guiload_text(text, target=w2, context=globals(), names=n2)
    w2.show()
    M.pump(120)

    check(set(n1) == set(n2) and n1, '文件入口与字符串入口名字表一致',
          (sorted(set(n1) ^ set(n2))[:8], len(n1), len(n2)))

    # id 会不同（各自分配），尺寸必须一致
    s1 = sizes(w1, n1, SAMPLES)
    s2 = sizes(w2, n2, SAMPLES)
    check(s1 == s2, '同名控件尺寸一致', (s1, s2))
    check(s1.get('BtnApply') and s1['BtnApply'][0] > 190,
          'BtnApply 宽度来自真实布局（>190）', s1.get('BtnApply'))

    # 空串必须明确报错，不能静默出个空窗口
    try:
        M.guiload_text('', target=None, context=globals(), names={})
        check(False, '空字符串应抛 RuntimeError')
    except RuntimeError as e:
        check('XML 字符串为空' in str(e), '空串报错信息明确', str(e))

    w1.close()
    w2.close()
    return finish('t_load_parity')


if __name__ == '__main__':
    sys.exit(main())
