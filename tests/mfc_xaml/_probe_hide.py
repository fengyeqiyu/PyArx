import sys, os, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import pp_app
M = __import__('PyMfcXaml')
cls = None
for n in dir(ppApp):
    o = getattr(ppApp, n)
    if isinstance(o, type) and hasattr(o, '_layout') and hasattr(o, 'onSplitDrag'):
        cls = o; break
app = cls(); app.win.show(); M.pump(80); app.showEditor(); M.pump(250)
cw, ch = app._clientSize()
def run(name, n=12):
    ts = []
    for i in range(n):
        w = 640 if i % 2 == 0 else 628
        x = cw - w - pp_app.SPLIT_W
        t0 = time.perf_counter(); app.onSplitDrag('up', x); M.pump(30)
        ts.append((time.perf_counter() - t0) * 1000.0)
    ts = sorted(ts)
    print('%-28s med %.1f ms (min %.1f max %.1f)' % (name, ts[len(ts)//2], ts[0], ts[-1]))
    return ts[len(ts)//2]
run('A 全开')
for ids, name in [([pp_app.MINI], 'B 关小地图'),
                  ([pp_app.MINI, pp_app.ST_PANEL], 'C +关状态栏'),
                  ([pp_app.MINI, pp_app.ST_PANEL, pp_app.ED_PANEL], 'D +关侧板白卡'),
                  ([pp_app.MINI, pp_app.ST_PANEL, pp_app.ED_PANEL, pp_app.TBL], 'E +关表格'),
                  ([pp_app.MINI, pp_app.ST_PANEL, pp_app.ED_PANEL, pp_app.TBL, pp_app.CV], 'F +关画布(只余装饰)')]:
    for i in ids:
        app.win.setVisible(int(i), False)
    M.pump(60)
    run(name)
os._exit(0)
