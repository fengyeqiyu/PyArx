# -*- coding: utf-8 -*-
"""power_plan_host.py —— 配电干线图 QML 版的 Python 宿主

职责：
  1) 给 QML 提供文件对话框与读写（QML 自己没有文件系统 UI）：QML 里 backend.call('io:xxx')
  2) XLSX 解析（纯标准库 zipfile + xml.etree，不依赖第三方库）
  3) 两种打开方式：
        open_in_cad()        挂到 AutoCAD 侧栏（走 PyArxCore 调色板）
        open_standalone()    独立 Win32 窗口 + PyArxQml.attach

QML 侧接口见 PowerPlan.qml 顶部注释；文本经根对象 payloadText 传递。
"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time
import zipfile
import xml.etree.ElementTree as ET
from xml.sax.saxutils import escape

QML_PATH = r'G:\PyArx\PyArxQml\qml\power_plan\PowerPlan.qml'
LOG_PATH = r'G:\PyArx\docs\_pyarx_qml_powerplan.log'

PANEL_NAME = '配电干线图'
PANEL_W = 1100
PANEL_H = 760

_dll_dir = r'G:\PyArx\X64\Release'


def _ensure_import_path():
    """pyd 依赖同目录 DLL，必须先 add_dll_directory，否则 ImportError: 找不到指定的模块"""
    src = r'G:\PyArx\PyArxQml'
    if src not in sys.path:
        sys.path.insert(0, src)
    for d in (_dll_dir, r'G:\PyArx\docs\_qt_baseline\mod'):
        if os.path.isdir(d):
            try:
                os.add_dll_directory(d)
            except (AttributeError, OSError):
                pass


def log(msg):
    line = '%s  %s\n' % (time.strftime('%H:%M:%S'), msg)
    try:
        with open(LOG_PATH, 'a', encoding='utf-8') as f:
            f.write(line)
    except OSError:
        pass


# ==================================================================
# Win32 文件对话框
# ==================================================================
OFN_OVERWRITEPROMPT = 0x00000002
OFN_PATHMUSTEXIST = 0x00000800
OFN_FILEMUSTEXIST = 0x00001000
OFN_NOCHANGEDIR = 0x00000008


class OPENFILENAMEW(ctypes.Structure):
    _fields_ = [
        ('lStructSize', wt.DWORD),
        ('hwndOwner', wt.HWND),
        ('hInstance', wt.HINSTANCE),
        ('lpstrFilter', wt.LPCWSTR),
        ('lpstrCustomFilter', wt.LPWSTR),
        ('nMaxCustFilter', wt.DWORD),
        ('nFilterIndex', wt.DWORD),
        ('lpstrFile', wt.LPWSTR),
        ('nMaxFile', wt.DWORD),
        ('lpstrFileTitle', wt.LPWSTR),
        ('nMaxFileTitle', wt.DWORD),
        ('lpstrInitialDir', wt.LPCWSTR),
        ('lpstrTitle', wt.LPCWSTR),
        ('Flags', wt.DWORD),
        ('nFileOffset', wt.WORD),
        ('nFileExtension', wt.WORD),
        ('lpstrDefExt', wt.LPCWSTR),
        ('lCustData', wt.LPARAM),
        ('lpfnHook', ctypes.c_void_p),
        ('lpTemplateName', wt.LPCWSTR),
        ('pvReserved', ctypes.c_void_p),
        ('dwReserved', wt.DWORD),
        ('FlagsEx', wt.DWORD),
    ]


def _run_dialog(title, filter_str, def_ext, save, owner=0, init_dir=None):
    buf = ctypes.create_unicode_buffer(4096)
    if save:
        buf.value = def_ext or ''
    ofn = OPENFILENAMEW()
    ofn.lStructSize = ctypes.sizeof(OPENFILENAMEW)
    ofn.hwndOwner = owner
    ofn.lpstrFilter = filter_str
    ofn.nFilterIndex = 1
    ofn.lpstrFile = ctypes.cast(buf, wt.LPWSTR)
    ofn.nMaxFile = 4096
    ofn.lpstrTitle = title
    ofn.lpstrInitialDir = init_dir or r'G:\配电规划设计'
    ofn.Flags = OFN_NOCHANGEDIR | (OFN_OVERWRITEPROMPT if save else
                                   (OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST))
    func = ctypes.windll.comdlg32.GetSaveFileNameW if save else ctypes.windll.comdlg32.GetOpenFileNameW
    if not func(ctypes.byref(ofn)):
        return ''
    return buf.value


def pick_open_csv():
    return _run_dialog('选择 CSV 文件', 'CSV 表格 (*.csv;*.txt)\x00*.csv;*.txt\x00所有文件 (*.*)\x00*.*\x00', 'csv', False)


def pick_open_xlsx():
    return _run_dialog('选择 XLSX 文件', 'Excel 工作簿 (*.xlsx)\x00*.xlsx\x00所有文件 (*.*)\x00*.*\x00', 'xlsx', False)


def pick_open_json():
    return _run_dialog('选择 JSON 文件', 'JSON (*.json)\x00*.json\x00所有文件 (*.*)\x00*.*\x00', 'json', False)


def pick_save(default_name, filt, ext, title):
    return _run_dialog(title, filt, default_name, True)


# ==================================================================
# XLSX → CSV 文本（标准库实现，只取第一个工作表）
# ==================================================================
_ALIAS = {
    '编号': 'code', 'code': 'code',
    '名称': 'name', 'name': 'name',
    '上游': 'upstream', 'upstream': 'upstream',
    '电缆型号': 'cable', 'cable': 'cable',
    '电压': 'voltage', 'voltage': 'voltage', '相数': 'voltage',
    '长度': 'length', '长度m': 'length', 'length': 'length', 'lengthm': 'length',
    '负载kw': 'loadKw', '负载': 'loadKw', 'loadkw': 'loadKw',
}
_NS = '{http://schemas.openxmlformats.org/spreadsheetml/2006/main}'


def _col_index(ref):
    letters = ''.join(ch for ch in ref if ch.isalpha())
    n = 0
    for ch in letters:
        n = n * 26 + (ord(ch) - 64)
    return n - 1


def _shared_strings(zf):
    try:
        raw = zf.read('xl/sharedStrings.xml')
    except KeyError:
        return []
    root = ET.fromstring(raw)
    out = []
    for si in root.iter(_NS + 'si'):
        parts = [t.text or '' for t in si.iter(_NS + 't')]
        out.append(''.join(parts))
    return out


def xlsx_to_csv_text(path):
    """读取第一个工作表，转成与 HTML 版一致的 CSV 文本（带表头）。"""
    with zipfile.ZipFile(path) as zf:
        shared = _shared_strings(zf)
        sheets = sorted([n for n in zf.namelist()
                         if n.startswith('xl/worksheets/sheet') and n.endswith('.xml')])
        if not sheets:
            raise ValueError('.xlsx 中未找到工作表')
        root = ET.fromstring(zf.read(sheets[0]))

    rows = []
    for r in root.iter(_NS + 'row'):
        arr = []
        for c in r.iter(_NS + 'c'):
            idx = _col_index(c.get('r') or '')
            t = c.get('t')
            if t == 's':
                v = c.find(_NS + 'v')
                val = shared[int(v.text)] if (v is not None and v.text) else ''
            elif t == 'inlineStr':
                val = ''.join(x.text or '' for x in c.iter(_NS + 't'))
            else:
                v = c.find(_NS + 'v')
                val = v.text if (v is not None and v.text) else ''
            if idx < 0:
                idx = len(arr)
            while len(arr) <= idx:
                arr.append('')
            arr[idx] = val
        rows.append(arr)

    rows = [r for r in rows if r and any(str(x).strip() for x in r)]
    if not rows:
        raise ValueError('工作表为空')

    header = [str(h).strip() for h in rows[0]]
    cols = [_ALIAS.get(h.lower()) or _ALIAS.get(h) for h in header]
    if 'code' not in cols:
        raise ValueError('缺少必需列「编号」')

    out = [['编号', '名称', '上游', '电缆型号', '负载kW', '电压', '长度m']]
    for r in rows[1:]:
        rec = {'code': '', 'name': '', 'upstream': '', 'cable': '',
               'loadKw': '', 'voltage': '', 'length': ''}
        for i, key in enumerate(cols):
            if key and i < len(r):
                rec[key] = str(r[i]).strip()
        out.append([rec['code'], rec['name'], rec['upstream'], rec['cable'],
                    rec['loadKw'], rec['voltage'], rec['length']])

    import csv
    import io
    sio = io.StringIO()
    w = csv.writer(sio, lineterminator='\r\n')
    w.writerows(out)
    return sio.getvalue()


# ==================================================================
# QML → Python 回调（backend.call）
# ==================================================================
def make_handler(qml_module):
    """返回 (op, a, b) -> str 的回调函数，交给 PyArxQml.set_callback。"""

    def get_payload():
        try:
            return qml_module.get_prop('payloadText') or ''
        except Exception as e:                     # noqa: BLE001
            log('读 payloadText 失败：%r' % (e,))
            return ''

    def handler(op, a=None, b=None):
        op = str(op)
        log('QML 请求：%s' % op)
        try:
            if op == 'io:importCsv':
                path = pick_open_csv()
                if not path:
                    return 'cancel'
                with open(path, 'r', encoding='utf-8-sig') as f:
                    text = f.read()
                qml_module.set_prop('payloadText', text)
                log('导入 CSV：%s（%d 字符）' % (path, len(text)))
                return 'ok'

            if op == 'io:importXlsx':
                path = pick_open_xlsx()
                if not path:
                    return 'cancel'
                text = xlsx_to_csv_text(path)
                qml_module.set_prop('payloadText', text)
                log('导入 XLSX：%s（转成 %d 字符 CSV）' % (path, len(text)))
                return 'ok'

            if op == 'io:importJson':
                path = pick_open_json()
                if not path:
                    return 'cancel'
                with open(path, 'r', encoding='utf-8-sig') as f:
                    text = f.read()
                qml_module.set_prop('payloadText', text)
                log('导入 JSON：%s' % path)
                return 'ok'

            if op in ('io:saveJson', 'io:saveCsv', 'io:saveSvg'):
                text = get_payload()
                if not text:
                    return 'err:没有可导出的内容'
                if op == 'io:saveJson':
                    default, filt, ext = 'power-plan.json', 'JSON (*.json)\x00*.json\x00', 'json'
                    title = '导出 JSON'
                elif op == 'io:saveCsv':
                    default, filt, ext = 'power-plan.csv', 'CSV (*.csv)\x00*.csv\x00', 'csv'
                    title = '导出 CSV'
                else:
                    default, filt, ext = 'power-plan.svg', 'SVG (*.svg)\x00*.svg\x00', 'svg'
                    title = '导出 SVG'
                path = pick_save(default, filt, ext, title)
                if not path:
                    return 'cancel'
                if op == 'io:saveCsv':
                    # CSV 已带 BOM，直接按 utf-8 写（避免 BOM 变双份）
                    with open(path, 'w', encoding='utf-8', newline='') as f:
                        f.write(text)
                else:
                    with open(path, 'w', encoding='utf-8', newline='') as f:
                        f.write(text)
                qml_module.set_prop('payloadText', path)
                log('已导出：%s（%d 字符）' % (path, len(text)))
                return 'ok'

            if op == 'io:savePng':
                path = pick_save('power-plan.png', 'PNG (*.png)\x00*.png\x00', 'png', '导出 PNG')
                if not path:
                    return 'cancel'
                qml_module.set_prop('payloadText', path)
                log('PNG 目标：%s' % path)
                return 'ok'

            return 'err:未识别的操作 ' + op
        except Exception as e:                     # noqa: BLE001
            log('处理 %s 失败：%r' % (op, e))
            return 'err:' + str(e)

    return handler


# ==================================================================
# 共用：建一个独立的顶层窗口（不用 CAD 侧栏页签）
# ==================================================================
_WINDOW_CLASS = 'PyArxPowerPlanHost'
_keep = {}          # 保住 hwnd / wndproc 引用，避免被 GC 掉


def _create_window(title, width, height, x=200, y=100):
    """建一个可缩放的顶层窗口，返回 hwnd。"""
    user32 = ctypes.windll.user32
    kernel32 = ctypes.windll.kernel32
    LR = ctypes.c_ssize_t
    WNDPROC = ctypes.WINFUNCTYPE(LR, wt.HWND, wt.UINT, ctypes.c_size_t, LR)
    user32.DefWindowProcW.restype = LR
    user32.DefWindowProcW.argtypes = [wt.HWND, wt.UINT, ctypes.c_size_t, LR]

    def _proc(hwnd, msg, wp, lp):
        if msg == 0x0010:                     # WM_CLOSE
            user32.DestroyWindow(hwnd)
            return 0
        return user32.DefWindowProcW(hwnd, msg, wp, lp)

    proc = WNDPROC(_proc)
    _keep['proc'] = proc                      # 必须持有，否则回调被回收

    class WNDCLASSEXW(ctypes.Structure):
        _fields_ = [
            ('cbSize', ctypes.c_uint), ('style', ctypes.c_uint),
            ('lpfnWndProc', ctypes.c_void_p), ('cbClsExtra', ctypes.c_int),
            ('cbWndExtra', ctypes.c_int), ('hInstance', wt.HANDLE),
            ('hIcon', wt.HANDLE), ('hCursor', wt.HANDLE),
            ('hbrBackground', wt.HANDLE), ('lpszMenuName', wt.LPCWSTR),
            ('lpszClassName', wt.LPCWSTR), ('hIconSm', wt.HANDLE),
        ]

    hinst = kernel32.GetModuleHandleW(None)
    wc = WNDCLASSEXW()
    wc.cbSize = ctypes.sizeof(wc)
    wc.lpfnWndProc = ctypes.cast(proc, ctypes.c_void_p)
    wc.hInstance = hinst
    wc.lpszClassName = _WINDOW_CLASS
    if not user32.RegisterClassExW(ctypes.byref(wc)) and not _keep.get('class_ok'):
        pass                                   # 已注册过同名类属于正常情况
    _keep['class_ok'] = True

    style = 0x00CF0000                         # WS_OVERLAPPEDWINDOW
    hwnd = user32.CreateWindowExW(0, _WINDOW_CLASS, title, style,
                                  x, y, width, height, None, None, hinst, None)
    if not hwnd:
        raise RuntimeError('CreateWindowExW 失败')
    user32.ShowWindow(hwnd, 5)                 # SW_SHOW
    user32.UpdateWindow(hwnd)
    _keep['hwnd'] = hwnd
    return hwnd


def _attach_panel(hwnd, width, height, log_path):
    import PyArxQml
    PyArxQml.set_callback(make_handler(PyArxQml))
    PyArxQml.set_log(log_path)
    ok = PyArxQml.attach(QML_PATH, hwnd, width, height)
    log('attach = %s（hwnd=0x%X）' % (ok, hwnd))
    if not ok:
        raise RuntimeError('PyArxQml.attach 失败，详见 %s' % log_path)
    PyArxQml.call('loadThreeLevel')
    log('已载入三级配电示例：%s' % PyArxQml.call('info'))
    return hwnd


# ==================================================================
# 打开方式 1：CAD 内独立浮窗（不再使用侧栏页签）
# ==================================================================
def open_floating(width=PANEL_W, height=PANEL_H, log_path=LOG_PATH):
    """在 CAD 进程内建一个独立顶层窗口承载面板。

    帧由 pyd 自己的泵定时器驱动（SetTimer 16ms，挂消息窗），所以这里不需要
    阻塞式的 pump 循环，函数返回后调用方可以立刻回到 CAD 命令行。
    """
    _ensure_import_path()
    hwnd = _create_window(PANEL_NAME, width, height)
    _attach_panel(hwnd, width, height, log_path)
    log('已打开独立浮窗：hwnd=0x%X' % hwnd)
    return hwnd


def close_floating():
    """关掉之前的浮窗（重复打开时用）。"""
    hwnd = _keep.get('hwnd')
    if hwnd:
        try:
            ctypes.windll.user32.PostMessageW(hwnd, 0x0010, 0, 0)
            log('已关闭浮窗 hwnd=0x%X' % hwnd)
        except Exception as e:                 # noqa: BLE001
            log('关闭浮窗失败：%r' % (e,))
        _keep['hwnd'] = None


# 兼容旧名字：脚本里写的 open_in_cad() 现在也走独立浮窗
open_in_cad = open_floating


# ==================================================================
# 打开方式 2：独立 Win32 窗口
# ==================================================================
def open_standalone(width=PANEL_W, height=PANEL_H, log_path=LOG_PATH):
    """独立 Python 进程里的窗口：建窗 + 挂 QML + 自己跑消息循环（直到窗口关闭）。"""
    _ensure_import_path()
    import PyArxQml

    user32 = ctypes.windll.user32
    hwnd = _create_window(PANEL_NAME, width, height)
    _attach_panel(hwnd, width, height, log_path)

    t0 = time.time()
    try:
        while user32.IsWindow(hwnd):
            PyArxQml.pump()
            time.sleep(0.012)
    except KeyboardInterrupt:
        pass
    finally:
        log('窗口已关闭，运行 %.1f 秒' % (time.time() - t0))
        if user32.IsWindow(hwnd):
            user32.DestroyWindow(hwnd)


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else 'standalone'
    if mode == 'cad':
        open_floating()
        print('已在 CAD 内打开独立浮窗（不阻塞命令行）')
    else:
        open_standalone()
