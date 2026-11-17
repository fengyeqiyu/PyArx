# -*- coding: utf-8 -*-
"""事件契约：control_events / event_schema 查询、不支持的绑定有诊断、严格模式抛错。"""
import os
import subprocess
import sys

from _kit import M, check, finish, ROOT, BIN

BAD_XML = ('<XWindow Width="240" Height="80">'
           '<XLabel Name="L" Left="8" Top="8" Width="200" Height="20" '
           'Text="标题" TextChanged="on_lab_change"/>'
           '</XWindow>')


def main():
    # 1) 契约表能查
    ev_lab = M.control_events('label')
    ev_tb = M.control_events('textbox')
    check(bool(ev_lab) and 'click' in ev_lab, 'label 有契约', ev_lab)
    check('change' in ev_tb, 'textbox 支持 change（XTextField 的 TextChanged 才算合法）', ev_tb)
    check('change' not in ev_lab, 'label 不产生 change（旧死绑定就栽在这）', ev_lab)
    schema = M.event_schema()
    import json
    data = json.loads(schema)
    check(len(data['events']) >= 14, 'event_schema 覆盖全部 15 种事件', len(data['events']))
    types = data.get('types') or {}
    check(len(types) >= 40, 'event_schema 带 type -> events 映射', len(types))
    check('change' in types.get('textfield', []), 'textfield 的契约含 change',
          types.get('textfield'))
    check('change' not in types.get('label', []), 'label 的契约不含 change',
          types.get('label'))

    # 2) 非严格模式：绑定照旧跳过，但要留诊断
    M.clear_diagnostics()
    path = os.path.join(ROOT, 'log', '_t_contract_bad.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(BAD_XML)
    win = M.guiload(path, context=globals(), names={})
    win.close()
    warns = M.event_warnings()
    check(any(w[1] == 'change' for w in warns), '非法绑定进 event_warnings', warns)

    # 3) 严格模式：加载就失败（环境变量是进程启动时读的，放子进程里验证）
    code = (
        'import os,sys\n'
        'sys.path.insert(0, %r)\n'
        'os.add_dll_directory(%r)\n'
        'os.environ["PYARX_D2D_RT"]="software"\n'
        'import PyMfcXml as M\n'
        'try:\n'
        '    M.guiload(%r, context=globals(), names={})\n'
        'except RuntimeError as e:\n'
        '    print("STRICT:", e)\n'
        '    sys.exit(0 if "不会产生该事件" in str(e) else 2)\n'
        'sys.exit(3)\n' % (BIN, BIN, path))
    env = dict(os.environ, PYARX_STRICT_XML='1', PYARX_D2D_RT='software')
    r = subprocess.run([sys.executable, '-c', code], capture_output=True, text=True,
                       env=env, timeout=180)
    check(r.returncode == 0, 'PYARX_STRICT_XML=1 时非法绑定抛 RuntimeError',
          (r.returncode, (r.stdout or '')[-200:], (r.stderr or '')[-200:]))

    M.clear_diagnostics()
    check(M.event_warnings() == [] and M.prop_warnings() == [], 'clear_diagnostics 清空')
    return finish('t_contract')


if __name__ == '__main__':
    sys.exit(main())
