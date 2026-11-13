import sys, os, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import pp_app
M = sys.modules.get('PyMfcXaml') or __import__('PyMfcXaml')

cls = None
for n in dir(ppApp):
    o = getattr(ppApp, n)
    if isinstance(o, type) and hasattr(o, '_layout') and hasattr(o, 'onSplitDrag'):
        cls = o; break
print('app class =', cls.__name__ if cls else None)
app = cls(); app.win.show(); M.pump(80); app.showEditor(); M.pump(250)
cw, ch = app._clientSize() if hasattr(app, '_clientSize') else (1324, 841)
w0 = app._sideWidth(cw) if hasattr(app, '_sideWidth') else 640

tl, tm, tp = [], [], []
base = w0
for i in range(30):
    target = base - (i % 6) * 4
    t0 = time.perf_counter(); app.onSplitDrag('move', target * 1.0); t1 = time.perf_counter()
    tl.append((t1 - t0) * 1000.0)
    # 单独量 moveMany（C++ 同步派发 WM_SIZE 也算在里面）
    t2 = time.perf_counter(); app._layout(); t3 = time.perf_counter(); tm.append((t3 - t2) * 1000.0)
    t4 = time.perf_counter(); M.pump(30); t5 = time.perf_counter(); tp.append((t5 - t4) * 1000.0)
def st(name, xs):
    xs = sorted(xs)
    print('%-22s n=%d  min %.1f  med %.1f  max %.1f ms' % (name, len(xs), xs[0], xs[len(xs)//2], xs[-1]))
print('侧板宽=%d' % w0)
st('onSplitDrag(含moveMany)', tl)
st('_layout 单独', tm)
st('pump(30)', tp)
print('三段合计 med = %.1f ms' % (sorted(tl)[15] + sorted(tm)[15] + sorted(tp)[15]))
os._exit(0)
