# -*- coding: utf-8 -*-
"""配电规划图 —— MFCXml 完整实现（对齐 power-plan.html 的功能全集）。

对照 HTML 的每一项：
  工具栏   图名可改 / 文档示例 / 三级配电示例 / 节点编辑 / 导入 CSV XLSX JSON /
           粘贴 JSON / 导出 JSON CSV SVG PNG / 缩小 100% 放大 / 适应窗口 /
           连线标注电缆 / 布局走向 / 搜索编号或名称
  画布     DwgCanvas 自绘（滚轮缩放、拖动平移、单击选中、双击折叠、选中联动高亮）
  小地图   缩略图 + 视口红框，点击跳转
  详情     编号 / 名称 / 上游 / 电缆 / 电压 / 负载 / 长度 / 下游数
  编辑     8 列表格 + 选中行表单 + 新增 / 删除 / 应用并重绘
  状态栏   节点 · 连线 · 交叉 · 电压统计 · 缩放比

运行：python ppApp.py
"""
import ctypes
import csv
import io
import json
import os
import sys
import time
import xml.etree.ElementTree as ET
import zipfile

DLL = r'G:\PyArx\X64\Release'
sys.path.insert(0, DLL)
os.add_dll_directory(DLL)
import PyMfcXml as M

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import engine
ENG = engine.PowerPlanEngine()      # 规则引擎：口径/文案/统计都从这里出

APP_W, APP_H = 1340, 880

# 窗口里真正的客户区（MFCXml 的 PyMfcNativeWindow 内区，四周留 8px 边）
CW, CH = APP_W - 16, 841

# ---- 与 HTML 同尺寸的版式常数（power-plan.html 的 CSS 实测值）--------------
# toolbar: padding 8 12 / 按钮 font-size 13 padding 5 10 → 行高 26；两行（1340 宽下 flex-wrap 换行）
TB_Y1, TB_Y2, TB_H = 8, 46, 25
CV_X, CV_Y = 8, 78
CV_W, CV_H = CW - 16, CH - CV_Y - 8
SURFACE_ON = os.environ.get('PYARX_SURFACE', '0') == '1'
PLAN_ZOOM_MIN = 0.05 if SURFACE_ON else 0.2
PLAN_ZOOM_MAX = 4.0
PLAN_FIT_CAP = 4.0 if SURFACE_ON else 1.0

# .detail：top 12 right 12 width 272（浮在画布右上）
DT_X, DT_Y, DT_W, DT_H = CW - 12 - 272, CV_Y + 12, 272, 346
# .mini：right 12 bottom 10，外框 padding 4 → 188x134，内 canvas 180x126
MI_FRAME = (CW - 12 - 188, CH - 10 - 134, 188, 134)
MI_CANVAS = (MI_FRAME[0] + 4, MI_FRAME[1] + 4, 180, 126)
# .status：left 12 bottom 10（宽度按 HTML 的长文案自适应，别被截断）
ST_X, ST_Y, ST_W, ST_H = 8, CH - 10 - 26, 1010, 26

# ---- 编辑侧板（.editor：贴右 640 宽、top 0 bottom 0，相对 canvas-wrap）------
ED_X, ED_Y, ED_W, ED_H = CW - 820, CV_Y, 820, CH - CV_Y
SPLIT = 1232                     # 画布与侧板之间的可拖动分割条（6px）
SPLIT_W = 6
L_SPLIT = 1233      # 分割线本身（1px owner-draw 细线，跟着分割条走）
SPLIT_MIN_W = 360        # 侧板最小宽：7 列表格的最小列宽 + 顶部 3 个按钮排得下
SPLIT_MAX_W = 900        # 侧板最大宽（人的偏好上限）
CV_MIN_W = 320           # 画布最小宽（再小就看不见图了）
ED_PAD = 10
ED_HEAD_H = 42                   # .editor-head padding 8px 10px + 26 按钮
ED_HEAD_H2 = 20                  # 表头行高
GRID_BASE = 4000                 # 行内编辑控件 id 池：GRID_BASE + row*8 + col
GRID_MAX_ROWS = 44               # 侧板高度能塞下的行数上限（行高最小 15）
GRID_RH = 22                    # 侧板行高（固定，不再为塞下而压扁；超出靠滚轮）

# ---- 控件 id -------------------------------------------------------------
E_TITLE = 1100
MINI_VIA_JSON = os.environ.get('PYARX_MINI_JSON', '') == '1'   # 缩略图走 JSON 重解析（对照用）
B_DOC, B_3L, B_EDIT = 1101, 1102, 1103
B_CSV, B_XLSX, B_JSON, B_PASTE = 1104, 1105, 1106, 1107
B_EJSON, B_ECSV, B_ESVG, B_EPNG = 1108, 1109, 1110, 1111
B_ZOUT, B_Z100, B_ZIN, B_FIT = 1112, 1113, 1114, 1115
C_CABLE, C_HORIZ = 1116, 1117
E_SEARCH, B_FIND = 1118, 1119
L_HINT = 1120
L_STATUS = 1121
B_DUAL = 1122                    # 双电源示例
B_EXLSX = 1123                   # 导出 XLSX
L_D0 = 1130                      # 旧详情 8 行（保留 id 段，已不用）
CV, MINI, TBL = 3001, 3011, 3021

# 详情浮层（照 .detail 的 DOM：h3 + 4 个 label/input + ne-row + ne-actions + ne-tip）
DT_PANEL, DT_TITLE, DT_TIP = 1220, 1221, 1222
DT_L0 = 1225                     # 编号 / 名称 / 上游 / 电缆型号 的 label：1225..1228
DT_LV, DT_LL, DT_LLOAD = 1229, 1230, 1231
D_F0 = 1240                      # 对应 4 个输入框：1240..1243
D_VOL, D_LEN, D_LOAD = 1244, 1245, 1246
D_SAVE, D_ADDCHILD, D_DEL = 1250, 1251, 1252

# 状态栏 / 小地图底板
ST_PANEL, MI_PANEL = 1260, 1261

# 编辑侧板
L_EDIT_TITLE, L_EDIT_COUNT = 1200, 1201
B_ADDROW, B_DELROW, B_LOADROW, B_APPLYROW, B_APPLY, B_CLOSE = 1202, 1203, 1204, 1205, 1206, 1207
E_F0 = 1210                      # 侧板表单（保留，供"读取/写入选中行"用）
ED_PANEL = 1217                  # 侧板白底
L_EDIT_FOOT = 1218
L_ED_H0 = 1300                    # 侧板表头 8 列标签

# 字段口径（用户 2026-11-12 定，对齐其电缆计算书）：
# 起点 / 回路 / 终点 / 型号规格 / 电缆根数 / 水平长度 / 垂直长度 / 余量 / 合计
# 合计 = ceil(水平 + 垂直 + 余量)；功率/电压等级仅用于画布卡片，舱在表格尾部
FIELDS = ['upstream', 'circuit', 'code', 'cable', 'voltage', 'strands',
          'hLen', 'vLen', 'margin', 'length', 'loadKw']
# 计算书导出的列（CSV/Excel 一比一照这张表，含电压等级）
BOOK_N = 11
COLS = list(ENG.rules.table.get('cols') or
            ['起点', '回路', '终点', '型号规格', '电压等级', '电缆根数',
             '水平长度', '垂直长度', '余量', '合计', '功率', ''])
# 表头显示用的短标题（Excel 列宽有限，长标题会被漏斗按钮挤成省略号）
HIDX_COLS = ['起点', '回路', '终点', '型号', '电压', '根数', '水平', '垂直', '余量', '合计', '功率', '']
COLW = list(ENG.rules.table.get('widths') or
            [104, 86, 78, 96, 68, 60, 64, 64, 44, 52, 44, 26])
NUM_COLS = ('strands', 'hLen', 'vLen', 'margin', 'length', 'loadKw')
SEP = ' · '
CFG = dict(boxW=132, boxH=48, termW=120, termH=44, nameH=8,
           gapX=30, gapY=56, margin=24, rootGap=64, colGap=100)


# ======================= 文件对话框（纯 ctypes） =======================
class OpenFileName(ctypes.Structure):
    """OPENFILENAMEW。第一个字段 lStructSize 绝对不能漏：
    漏了 Windows 直接返回 0，CommDlgExtendedError = 1(CDERR_STRUCTSIZE)，
    对话框根本不弹，表现就是「点按钮没反应」。"""
    _fields_ = [
        ('lStructSize', ctypes.c_uint32),
        ('hwndOwner', ctypes.c_void_p), ('hInstance', ctypes.c_void_p),
        ('lpstrFilter', ctypes.c_wchar_p), ('lpstrCustomFilter', ctypes.c_wchar_p),
        ('nMaxCustFilter', ctypes.c_uint32), ('nFilterIndex', ctypes.c_uint32),
        ('lpstrFile', ctypes.c_wchar_p), ('nMaxFile', ctypes.c_uint32),
        ('lpstrFileTitle', ctypes.c_wchar_p), ('nMaxFileTitle', ctypes.c_uint32),
        ('lpstrInitialDir', ctypes.c_wchar_p), ('lpstrTitle', ctypes.c_wchar_p),
        ('Flags', ctypes.c_uint32), ('nFileOffset', ctypes.c_uint16),
        ('nFileExtension', ctypes.c_uint16), ('lpstrDefExt', ctypes.c_wchar_p),
        ('lCustData', ctypes.c_void_p), ('lpfnHook', ctypes.c_void_p),
        ('lpTemplateName', ctypes.c_wchar_p),
        ('pvReserved', ctypes.c_void_p), ('dwReserved', ctypes.c_uint32),
        ('FlagsEx', ctypes.c_uint32),
    ]


def fileDialog(save, title, flt, default_name='', owner=None):
    """flt 形如 'CSV 文件 (*.csv)|*.csv|所有文件 (*.*)|*.*'
    owner 必须传主窗口 hwnd：本程序窗口是 TOPMOST，无主对话框会被压在它下面，
    用户看起来就是「点了按钮没反应」。"""
    buf = ctypes.create_unicode_buffer(1024)
    if default_name:
        buf.value = default_name
    ofn = OpenFileName()
    ofn.lStructSize = ctypes.sizeof(OpenFileName)
    ofn.hwndOwner = owner
    ofn.lpstrFilter = flt.replace('|', '\0') + '\0'
    ofn.lpstrFile = ctypes.cast(buf, ctypes.c_wchar_p)
    ofn.nMaxFile = 1024
    ofn.lpstrTitle = title
    ofn.lpstrInitialDir = r'G:\PyArx\docs'
    ofn.lpstrDefExt = flt.split('|')[1].split('*')[-1].lstrip('.') if save else None
    ofn.Flags = 0x00080000 | 0x00001000 | 0x00000008   # EXPLORER | FILEMUSTEXIST | PATHMUSTEXIST
    if save:
        ofn.Flags |= 0x00000002   # OVERWRITEPROMPT
    fn = ctypes.windll.comdlg32.GetSaveFileNameW if save else ctypes.windll.comdlg32.GetOpenFileNameW
    if not fn(ctypes.byref(ofn)):
        return ''
    return buf.value


def clipboardText():
    """读 CF_UNICODETEXT。必须显式声明 restype/argtypes：ctypes 默认把返回值当 int，
    64 位下句柄会被截成 32 位，GlobalLock 出来的指针一读就 access violation。"""
    u = ctypes.windll.user32
    k = ctypes.windll.kernel32
    u.GetClipboardData.restype = ctypes.c_void_p
    u.GetClipboardData.argtypes = [ctypes.c_uint]
    k.GlobalLock.restype = ctypes.c_void_p
    k.GlobalLock.argtypes = [ctypes.c_void_p]
    k.GlobalUnlock.argtypes = [ctypes.c_void_p]
    if not u.OpenClipboard(None):
        return ''
    try:
        h = u.GetClipboardData(13)      # CF_UNICODETEXT
        if not h:
            return ''
        p = k.GlobalLock(h)
        if not p:
            return ''
        try:
            return ctypes.wstring_at(p)
        finally:
            k.GlobalUnlock(h)
    finally:
        u.CloseClipboard()


# ======================= XLSX → 行 =======================
NS = '{http://schemas.openxmlformats.org/spreadsheetml/2006/main}'


def colIndex(ref):
    n = 0
    for ch in ref:
        if 'A' <= ch <= 'Z':
            n = n * 26 + (ord(ch) - 64)
        else:
            break
    return n - 1


def colName(i):
    """列号 → Excel 列名（0→A, 26→AA）。"""
    s = ''
    i += 1
    while i:
        i, r = divmod(i - 1, 26)
        s = chr(65 + r) + s
    return s


def writeXlsx(path, rows):
    """把二维行写成 .xlsx。优先用 openpyxl（表头加粗、冻结首行、按内容调列宽、
    数字写成数值单元格）；openpyxl 不可用时退回手搓的最小 OOXML 包。"""
    rows = [list(r) for r in rows if r]
    if not rows:
        return
    try:
        from openpyxl import Workbook
        from openpyxl.styles import Alignment, Font
    except Exception:
        return _writeXlsxManual(path, rows)

    wb = Workbook()
    ws = wb.active
    ws.title = '配电规划'
    for r in rows:
        ws.append([_cellVal(v) for v in r])
    for c in ws[1]:
        c.font = Font(bold=True)
        c.alignment = Alignment(horizontal='center', vertical='center')
    ws.freeze_panes = 'A2'
    for ci, col in enumerate(ws.iter_cols(min_row=1, max_row=ws.max_row), start=1):
        w = 8
        for cell in col:
            s = '' if cell.value is None else str(cell.value)
            u = sum(2 if ord(ch) > 0x2E7F else 1 for ch in s)
            w = max(w, u + 2)
        ws.column_dimensions[colName(ci - 1)].width = min(w, 40)
    wb.save(path)


def _cellVal(v):
    """数字串转数值（Excel 里能直接算），其余原样；空串不写单元格。"""
    if v is None or str(v) == '':
        return None
    s = str(v)
    try:
        f = float(s)
    except Exception:
        return s
    return int(f) if f == int(f) and '.' not in s else f


def _writeXlsxManual(path, rows):
    """没有 openpyxl 时的兜底：手搓最小 OOXML 包。"""
    def esc(s):
        return (str(s).replace('&', '&amp;').replace('<', '&lt;')
                .replace('>', '&gt;').replace('"', '&quot;'))

    sheet = ['<?xml version="1.0" encoding="UTF-8" standalone="yes"?>',
             '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">',
             '<sheetData>']
    for r, row in enumerate(rows, start=1):
        sheet.append('<row r="%d">' % r)
        for c, val in enumerate(row):
            if val is None or str(val) == '':
                continue
            ref = '%s%d' % (colName(c), r)
            v = str(val)
            try:
                float(v)
                sheet.append('<c r="%s" t="n"><v>%s</v></c>' % (ref, esc(v)))
            except Exception:
                sheet.append('<c r="%s" t="inlineStr"><is><t>%s</t></is></c>'
                             % (ref, esc(v)))
        sheet.append('</row>')
    sheet.append('</sheetData></worksheet>')

    ct = ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
          '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
          '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
          '<Default Extension="xml" ContentType="application/xml"/>'
          '<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>'
          '<Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>'
          '</Types>')
    rels = ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>'
            '</Relationships>')
    wb = ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
          '<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" '
          'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">'
          '<sheets><sheet name="配电规划" sheetId="1" r:id="rId1"/></sheets></workbook>')
    wbr = ('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
           '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
           '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>'
           '</Relationships>')
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as z:
        z.writestr('[Content_Types].xml', ct)
        z.writestr('_rels/.rels', rels)
        z.writestr('xl/workbook.xml', wb)
        z.writestr('xl/_rels/workbook.xml.rels', wbr)
        z.writestr('xl/worksheets/sheet1.xml', '\n'.join(sheet))


def readXlsx(path):
    with zipfile.ZipFile(path) as z:
        names = z.namelist()
        shared = []
        if 'xl/sharedStrings.xml' in names:
            root = ET.fromstring(z.read('xl/sharedStrings.xml'))
            for si in root.findall(NS + 'si'):
                shared.append(''.join(t.text or '' for t in si.iter(NS + 't')))
        sheet = None
        for nm in names:
            if nm.startswith('xl/worksheets/sheet') and nm.endswith('.xml'):
                sheet = nm
                break
        if sheet is None:
            return []
        root = ET.fromstring(z.read(sheet))
    rows = []
    for row in root.iter(NS + 'row'):
        cells = {}
        for c in row.findall(NS + 'c'):
            ci = colIndex(c.get('r') or 'A1')
            t = c.get('t')
            v = c.find(NS + 'v')
            if t == 'inlineStr':
                isv = c.find(NS + 'is')
                txt = ''.join(x.text or '' for x in isv.iter(NS + 't')) if isv is not None else ''
            elif t == 's':
                txt = shared[int(v.text)] if v is not None and v.text else ''
            else:
                txt = (v.text or '') if v is not None else ''
            cells[ci] = txt
        if cells:
            n = max(cells) + 1
            rows.append([cells.get(i, '') for i in range(n)])
    return rows


def rowsToCsv(rows):
    out = []
    for r in rows:
        cells = []
        for v in r:
            v = (v or '').replace('"', '""')
            cells.append('"%s"' % v if (',' in v or '"' in v) else v)
        out.append(','.join(cells))
    return '\r\n'.join(out) + '\r\n'


# ======================= 示例数据 =======================
def mkNode(code, name, up, cable, load, volt, length,
           circuit='', strands=1, hl=None, vl=0, margin=2):
    """示例节点：余量默认 2 m（与计算书一致），合计 = ceil(水平+垂直+余量)。"""
    h = float(length if hl is None else hl)
    v = float(vl or 0)
    m = float(margin or 0)
    return dict(code=code, name=name, upstream=up,
                circuit=circuit or ('WL-' + code), cable=cable,
                loadKw=load, voltage=volt, strands=int(strands),
                hLen=h, vLen=v, margin=m,
                length=engine.calc_length(h, v, m))


def sampleDoc():
    """文档示例（13 节点，与 HTML SAMPLE_CSV 同构）。"""
    return [
        mkNode('TR1', '1号变压器', '', '', 0, '10kV', 0, 'WL-TR1'),
        mkNode('AT1', '1号配电柜', 'TR1', 'YJV22-4×95', 0, '380', 12.5, 'WL-AT1'),
        mkNode('AT1-1', '照明回路', 'AT1', 'YJV22-4×35', 9, '380', 8, 'WL-AT1-1'),
        mkNode('AT1-1-1', '一层照明', 'AT1-1', 'BV-3×6', 3, '220', 5, 'WL-AT1-1-1'),
        mkNode('AT1-1-2', '二层照明', 'AT1-1', 'BV-3×6', 4, '220', 7, 'WL-AT1-1-2'),
        mkNode('AT1-1-3', '应急照明', 'AT1-1', 'BV-3×6', 5, '220', 9, 'WL-AT1-1-3'),
        mkNode('AT1-2', '动力回路', 'AT1', 'YJV22-4×35', 22, '380', 10, 'WL-AT1-2'),
        mkNode('AT1-2-1', '水泵', 'AT1-2', 'BV-4×10', 7.5, '380', 6, 'WL-AT1-2-1'),
        mkNode('AT1-2-2', '风机', 'AT1-2', 'BV-4×10', 5.5, '380', 6, 'WL-AT1-2-2'),
        mkNode('AT2', '2号配电柜', 'TR1', 'YJV22-4×95', 0, '380', 14, 'WL-AT2'),
        mkNode('AT2-1', '插座回路', 'AT2', 'YJV22-4×35', 12, '380', 9, 'WL-AT2-1'),
        mkNode('AT2-1-1', '一层插座', 'AT2-1', 'BV-3×6', 4, '220', 6, 'WL-AT2-1-1'),
        mkNode('AT2-1-2', '二层插座', 'AT2-1', 'BV-3×6', 4, '220', 6, 'WL-AT2-1-2'),
    ]


def sampleDual():
    """双电源示例：AT2 柜两条进线（TR1 主 / TR2 备用），
    表格里同样两行——取电点、回路编号、长度各填各的，卡片仍只有一张。"""
    nodes = [
        mkNode('TR1', '1号变压器', '', '', 0, '10kV', 0, 'WL-TR1'),
        mkNode('TR2', '2号变压器', '', '', 0, '10kV', 0, 'WL-TR2'),
        mkNode('AT1', '1号配电柜', 'TR1', 'YJV22-4×95', 0, '380', 12.5, 'WL-AT1'),
    ]
    for j in (1, 2):
        nodes.append(mkNode('AT1-%d' % j, '1号柜%d号回路' % j, 'AT1',
                            'YJV22-4×35', 10 + j, '380', 6 + j, 'WL-AT1-%d' % j))
    # 双电源柜：feeds 两条进线，各自回路编号 + 水平/垂直长度
    at2 = mkNode('AT2', '双电源柜 2ATS', '', 'YJV22-4×35', 11, '380', 7,
                 'WL-AT2-1', 1, 5, 2)
    at2['feeds'] = [
        {'upstream': 'TR1', 'circuit': 'WL-AT2-1', 'cable': 'YJV22-4×35',
         'strands': 1, 'hLen': 5, 'vLen': 2, 'margin': 2,
         'length': engine.calc_length(5, 2, 2)},
        {'upstream': 'TR2', 'circuit': 'WL-AT2-2', 'cable': 'YJV22-4×35',
         'strands': 1, 'hLen': 6, 'vLen': 3, 'margin': 2,
         'length': engine.calc_length(6, 3, 2)},
    ]
    nodes.append(at2)
    for j in (1, 2, 3):
        nodes.append(mkNode('AT2-%d' % j, '2号柜%d号回路' % j, 'AT2',
                            'YJV22-4×25', 9 + j, '380', 5 + j, 'WL-AT2-%d' % (j + 2)))
        for k in range(1, 4):
            nodes.append(mkNode('AT2-%d-%d' % (j, k), '', 'AT2-%d' % j,
                                'BV-3×4', 1 + k, '220' if k <= 2 else '380',
                                2 + k * 2, 'WL-AT2-%d-%d' % (j, k + 2)))
    return nodes


def sampleThreeLevel():
    """三级配电示例（39 节点，与 HTML btn3Level 同构）。"""
    nodes = [mkNode('TR1', '1号变压器', '', '', 0, '10kV', 0, 'WL-TR1')]
    for i in (1, 2):
        nodes.append(mkNode('AT%d' % i, '%d号配电柜' % i, 'TR1', 'YJV22-4×95',
                            0, '380', 12.5, 'WL-AT%d' % i))
        for j in (1, 2, 3):
            code = 'AT%d-%d' % (i, j)
            nodes.append(mkNode(code, '%d号柜%d号回路' % (i, j), 'AT%d' % i,
                                'YJV22-4×35', 10 + j, '380', 6 + j, 'WL-' + code))
            for k in range(1, 6):
                nodes.append(mkNode('%s-%d' % (code, k), '', code, 'BV-3×6',
                                    2 + k, '220' if k <= 3 else '380',
                                    3 + k * 2, 'WL-%s-%d' % (code, k)))
    return nodes

def sampleBigCsv(n1=20, n2=10, n3=50):
    """超大数据示例（1 + n1 + n1*n2 + n1*n2*n3 行，默认 10221 行），
    口径与电缆计算书 11 列一致，合计 = ceil(水平+垂直+余量)。"""
    import math
    out = ['起点,回路,终点,型号规格,电压等级,电缆根数,水平长度,垂直长度,余量,合计,功率']
    out.append(',WL-GRID,GRID,-,10kV 中压,1,0,0,0,0,0')
    for i in range(1, n1 + 1):
        h, v, m = 3, 2, 2
        out.append('GRID,WL-TR%d,TR%d,YJV22-4×95,10kV/380V 三相,1,%d,%d,%d,%d,1.5'
                   % (i, i, h, v, m, math.ceil(h + v + m)))
    for i in range(1, n1 + 1):
        for j in range(1, n2 + 1):
            h, v, m = 4 + j * 2, 2, 2
            out.append('TR%d,WL-AT%d-%02d,AT%d-%d,YJV22-4×35,380V 三相,1,%d,%d,%d,%d,%.1f'
                       % (i, i, j, i, j, h, v, m, math.ceil(h + v + m), 3.0 * j))
    for i in range(1, n1 + 1):
        for j in range(1, n2 + 1):
            for k in range(1, n3 + 1):
                h, v, m = 5 + (k % 8) * 2, 0, 2
                out.append('AT%d-%d,WL-AT%d-%d-%d,AT%d-%d-%d,BV-3×6,%s,1,%d,%d,%d,%d,%.1f'
                           % (i, j, i, j, k, i, j, k,
                              '220V 单相' if k % 2 else '380V 三相',
                              h, v, m, math.ceil(h + v + m), 1.5 + (k % 5) * 0.5))
    return '\n'.join(out) + '\n'


def normVolt(v):
    """规则引擎口径（HTML normVoltage）；参数在 powerplan.rules.json。"""
    return ENG.normVoltage(v)


def voltBucket(v):
    return ENG.voltBucket(v)


def voltText(v):
    """HTML voltText：220V 单相 / 380V 三相 / 其余 <v> 中压|高压。"""
    return ENG.voltText(v)


# ======================= 应用 =======================
class App:
    def __init__(self):
        self.win = M.Window('配电规划图 — MFCXml', APP_W, APP_H)
        self.win.set_theme(False)
        self._new_table = hasattr(self.win, 'addTable')     # 新自绘表格 PyMfcTable（旧 SysListView32 路径保留兼容）
        self.nodes = sampleThreeLevel()
        self.collapsed = set()
        self.horiz = True
        self.showCable = True
        self.titleText = '三级配电干线图'
        self.view = None            # 最近一次 build 结果
        self.editing = False
        self.curSel = ''
        self._row_edit_open = False           # 行编辑条是否展开（默认关，双击改走原地编辑）
        self._row_edit_row = -1               # 正在编辑的行号
        self._ce = None                       # 原地单元格编辑：(行, 列, 编辑框 id)
        self._ed_w = ED_W                     # 侧板当前宽度（拖动分割条可改，360..900）
        self._tbl_pos = (0, 0)                # 表格左上角在侧板内的偏移
        self._detail_visible = False          # 详情浮层是否可见（layout 要用它决定显隐）
        self._detail_raised = False           # 详情 z 序是否已置顶（重复弹层跳过 raiseMany）
        self._colw_now = None                 # 当前列宽（随侧板宽度变化重算）
        self._vol_opts_seen = None            # 详情电压下拉已灌过的选项键（避免逐节点重建串）
        self._sort_col, self._sort_asc = -1, True   # 点同一列二次反向
        self.cvW, self.cvH = CV_W, CV_H     # 画布当前尺寸（编辑侧板打开时会变窄）
        self._grid_ids = []        # 侧板行内编辑控件 id（每次 fillTable 重建）
        self._grid_rows = 0
        self.buildUi()
        # 旧测试/探针保留的兼容名：新代码用 snake_case，旧用例经这些别名进入同一实现。

    @property
    def _newTable(self):
        return self._new_table

    @property
    def _edW(self):
        return self._ed_w

    @_edW.setter
    def _edW(self, value):
        self._ed_w = value

    @property
    def _tblPos(self):
        return self._tbl_pos

    @property
    def _edRc(self):
        return self._ed_rc

    @property
    def _detailVisible(self):
        return self._detail_visible

    def _sideBounds(self, cw=None):
        return self.sideBounds(cw)

    def _sideWidth(self, cw=None):
        return self.sideWidth(cw)

    def _cellBegin(self, row, col):
        return self.cellBegin(row, col)

    def _cellEnd(self, commit=True):
        return self.cellEnd(commit)

    def _rowEditClose(self):
        return self.rowEditClose()

    def _sceneView(self):
        return self.sceneView()

    def _sceneText(self):
        return self.sceneText()

    # ---------- UI ----------
    def buildUi(self):
        w = self.win
        y1, y2, bh = TB_Y1, TB_Y2, TB_H
        face = 'Microsoft YaHei'
        self._font_ids = []

        def btn(bid, text, x, bw, y=None):
            yy = y1 if y is None else y
            w.add_button(bid, text, x, yy, bw, bh)
            self._font_ids.append((bid, 13, False))
            return x + bw + 8

        # 第一行：h1(图名) + 数据按钮 + 导出组（顺序与 HTML 一致）
        w.add_edit(E_TITLE, self.titleText, 12, y1, 130, bh)
        self._font_ids.append((E_TITLE, 15, True))
        x = 12 + 130 + 8
        x = btn(B_DOC, '文档示例', x, 78)
        x = btn(B_3L, '三级配电示例', x, 106)
        x = btn(B_DUAL, '双电源示例', x, 92)
        x = btn(B_EDIT, '节点编辑', x, 76)
        x = btn(B_CSV, '导入 CSV', x, 78)
        x = btn(B_XLSX, '导入 XLSX', x, 86)
        x = btn(B_JSON, '导入 JSON', x, 90)
        x = btn(B_PASTE, '粘贴 JSON', x, 90)
        # .group 的 gap 是 4
        x += 2
        x = btn(B_EJSON, '导出 JSON', x, 88) - 8 + 4
        x = btn(B_ECSV, '导出 CSV', x, 82) - 8 + 4
        x = btn(B_EXLSX, '导出 XLSX', x, 88) - 8 + 4
        x = btn(B_ESVG, '导出 SVG', x, 82) - 8 + 4
        btn(B_EPNG, '导出 PNG', x, 82)

        # 第二行：缩放组 + 连线标注 + 布局下拉 + 搜索框
        x = 12
        x = btn(B_ZOUT, '－', x, 34, y2) - 8 + 4
        x = btn(B_Z100, '100%', x, 62, y2) - 8 + 4
        x = btn(B_ZIN, '＋', x, 34, y2) - 8 + 4
        x = btn(B_FIT, '适应窗口', x, 78, y2) + 4
        w.add_check_box(C_CABLE, '连线标注电缆', x, y2, 116, bh)
        self._font_ids.append((C_CABLE, 13, False))
        w.set_check(C_CABLE, True)
        x += 116 + 8
        w.add_label(L_HINT, '布局', x, y2 + 4, 34, 20)
        self._font_ids.append((L_HINT, 13, False))
        x += 34
        w.add_combo(C_HORIZ, x, y2, 112, bh)
        w.add_string(C_HORIZ, '横向 左→右')
        w.add_string(C_HORIZ, '纵向 上→下')
        w.set_cur_sel(C_HORIZ, 0)
        self._font_ids.append((C_HORIZ, 13, False))
        x += 112 + 8
        w.add_edit(E_SEARCH, '', x, y2, 170, bh)     # 输入即搜（HTML 的 input 事件）
        self._font_ids.append((E_SEARCH, 13, False))

        # 画布：全宽，工具栏下方到底部
        w.add_dwg_canvas(CV, CV_X, CV_Y, CV_W, CV_H)

        # 状态栏：浮在画布左下（白底圆角近似）
        w.add_white_rect(ST_PANEL, ST_X, ST_Y, ST_W, ST_H)   # HTML .status 无阴影
        w.add_label(L_STATUS, '拖动平移 · 滚轮缩放', ST_X + 4, ST_Y + 4, ST_W - 8, 18)
        self._font_ids.append((L_STATUS, 12, False))

        # 小地图：外框 188×134 + 内 canvas 180×126，浮在画布右下
        # HTML .mini: box-shadow 0 2px 8px rgba(16,24,40,.08)，radius 4
        w.add_shadow_box(MI_PANEL, MI_FRAME[0], MI_FRAME[1], MI_FRAME[2], MI_FRAME[3],
                       10, 4, 0, 2, 8, '#14101828', '#FFFFFF', '#FFFFFF', '#E4E7EC', 1)
        w.add_dwg_canvas(MINI, *MI_CANVAS)
        w.set_dwg_options(MINI, False, True, CV)

        # 详情浮层：默认隐藏，选中节点时出现在画布右上 272 宽
        self.buildDetail()

        # 编辑侧板（默认隐藏）：640 宽贴右，头部 3 按钮、表格、foot 说明
        self.buildEditor()

        # 回调
        w.on_click(B_DOC, self.onSampleDoc)
        w.on_click(B_3L, self.onThreeLevel)
        w.on_click(B_DUAL, self.onDualSource)
        w.on_click(B_EDIT, self.onToggleEditor)
        w.on_click(B_CSV, self.onImportCsv)
        w.on_click(B_XLSX, self.onImportXlsx)
        w.on_click(B_JSON, self.onImportJson)
        w.on_click(B_PASTE, self.onPasteJson)
        w.on_click(B_EJSON, self.onExportJson)
        w.on_click(B_ECSV, self.onExportCsv)
        w.on_click(B_EXLSX, self.onExportXlsx)
        w.on_click(B_ESVG, self.onExportSvg)
        w.on_click(B_EPNG, self.onExportPng)
        w.on_click(B_ZOUT, lambda: self.zoomAt(1.0 / 1.25))
        w.on_click(B_ZIN, lambda: self.zoomAt(1.25))
        w.on_click(B_Z100, lambda: self.setZoom(1.0))
        w.on_click(B_FIT, self.fitView)
        w.on_change(E_TITLE, self.onTitleChange)
        w.on_change(E_SEARCH, self.onSearchChange)
        w.on_checked(C_CABLE, self.onCableToggle)
        w.on_change(C_HORIZ, self.onDirChange)
        # 单击：C++ 画布做完命中（卡片或连线）后回调，Python 取回选中编号弹详情
        w.on_click(CV, self.onCanvasClick)
        w.on_double_click(CV, self.onCanvasDblclick)
        w.on_click(MINI, self.onMiniClick)
        w.on_resize(self.on_resize)    # 窗口尺寸变化 → 按网格重排（见 layout）

        self.hideEditor()
        self.hideDetail()
        self.rebuild(fit=True)
        self.applyFonts()

    # ---------- 版式（grid 自适应） ----------
    def clientSize(self):
        try:
            cw, ch = self.win.client_size()
        except Exception:
            cw, ch = CW, CH
        if not cw or not ch or cw < 400 or ch < 300:
            cw, ch = CW, CH           # 首帧窗口还没真正布局出来，先用设计基准
        return int(cw), int(ch)

    def sideBounds(self, cw=None):
        """侧板宽度的合法区间。上限不只由偏好决定，还要给画布留 CV_MIN_W：
        窗口越窄，上限越低；窗口窄到连下限都放不下时退回下限。"""
        if cw is None:
            cw, _ = self.client_size()
        room = cw - CV_X - 8 - SPLIT_W - CV_MIN_W        # 扣掉画布最小宽后剩下的空间
        lo = SPLIT_MIN_W
        hi = max(lo, min(SPLIT_MAX_W, int(room)))
        return lo, hi

    def sideWidth(self, cw=None):
        """当前**生效**的侧板宽度 = 用户偏好 _ed_w 在合法区间里钳一次。
        窗口临时变窄只收窄生效宽度、不动偏好，窗口恢复后自动回到偏好宽度。"""
        lo, hi = self.sideBounds(cw)
        return max(lo, min(hi, int(self._ed_w)))

    def layout(self, drag=False):
        """整窗按网格重排：
          行：工具栏（固定 78 高）/ 内容区（*）/ 底边 8
          列：画布（*）/ 侧板（640，仅编辑时占位）
        窗口放大时新增空间全部给画布，和 HTML 的 flex/grid 行为一致。"""
        w = self.win
        cw, ch = self.client_size()
        ed_w = (self.sideWidth(cw) + SPLIT_W) if self.editing else 0
        cvW = max(CV_MIN_W, cw - CV_X - 8 - ed_w)
        cvH = max(200, ch - CV_Y - 8)
        self.cvW, self.cvH = cvW, cvH

        items = [(CV, CV_X, CV_Y, cvW, cvH, True)]

        # 状态栏：左 + 底 锚定（HTML .status left 12 / bottom 10）
        if not drag:                     # 拖动分割条时 cw/ch 没变，这两块不用每帧搬
            st_w = max(240, min(ST_W, cvW - 8))
            st_y = ch - 10 - ST_H
            items += [(ST_PANEL, ST_X, st_y, st_w, ST_H, True),
                      (L_STATUS, ST_X + 4, st_y + 4, st_w - 8, 18, True)]

            # 小地图：右 + 底 锚定（HTML .mini right 12 / bottom 10）
            mfx = cw - 12 - MI_FRAME[2]
            mfy = ch - 10 - MI_FRAME[3]
            items += [(MI_PANEL, mfx, mfy, MI_FRAME[2], MI_FRAME[3], True),
                      (MINI, mfx + 4, mfy + 4, MI_CANVAS[2], MI_CANVAS[3], True)]

        if self.editing:
            items += self.layoutEditor(cw, ch)

        # 详情浮层：画布右上（HTML .detail top 12 right 12）
        if getattr(self, '_detail_rc', None):
            vis = bool(getattr(self, '_detail_visible', False))
            dtx = CV_X + cvW - 12 - DT_W
            dty = CV_Y + 12
            for cid, (x, y, ww, hh) in self._detail_rc.items():
                items.append((cid, dtx + (x - DT_X), dty + (y - DT_Y), ww, hh, vis))
        if items:
            w.move_many(items)

    def layoutEditor(self, cw, ch):
        """侧板内部：头部（标题 / 3 按钮）→ 表格（原生 ListView 撑满）→ 行编辑条 → footer。"""
        edw = self.sideWidth(cw)
        ed_x = cw - edw                                  # 右贴窗口
        if ed_x < CV_X + CV_MIN_W + SPLIT_W:             # 极端窄窗兜底：先保画布最小宽
            ed_x = CV_X + CV_MIN_W + SPLIT_W
            edw = max(240, cw - ed_x)
        ed_y = CV_Y
        ed_h = max(240, ch - CV_Y - 8)
        self._ed_rc = (ed_x, ed_y, edw, ed_h)
        px = ed_x + ED_PAD
        inner = edw - 2 * ED_PAD
        on = self._row_edit_open

        foot_y = ed_y + ed_h - 30
        strip_y = foot_y - 10 - 56                     # 行编辑条：两行（输入 / 按钮）
        items = [
            (ED_PANEL, ed_x, ed_y, edw, ed_h, True),
            (SPLIT, ed_x - SPLIT_W, ed_y, SPLIT_W, ed_h, True),
            (L_EDIT_TITLE, px, ed_y + 12, 120, 20, True),
            (L_EDIT_COUNT, px + 78, ed_y + 12, 160, 20, True),
            (B_CLOSE, ed_x + edw - ED_PAD - 72, ed_y + 8, 72, 26, True),
            (B_APPLY, ed_x + edw - ED_PAD - 172, ed_y + 8, 96, 26, True),
            (B_ADDROW, ed_x + edw - ED_PAD - 268, ed_y + 8, 92, 26, True),
            (L_EDIT_FOOT, px, foot_y, inner, 20, True),
        ]
        # 列宽按可用宽度等比缩放（末列 ✕ 固定 26），避免不该出现的横向滚动条
        widths = self.gridColumns(inner)

        if widths != self._colw_now:
            self._colw_now = widths
            if self._new_table:
                self.win.set_table_columns(TBL, widths[:len(FIELDS) + 1], HIDX_COLS)
                self.win.set_table_col_types(TBL, [0, 0, 0, 0, 2, 0, 2])
            elif hasattr(self.win, 'setColumnWidths'):
                self.win.set_column_widths(TBL, widths)
            elif hasattr(self.win, 'setColumnWidth'):
                for i, pxw in enumerate(widths):
                    self.win.set_column_width(TBL, i, pxw)
        # 行编辑条：7 个输入框必须用"缩放后的列宽"排，才能和表格的列一一对齐；
        # 用原始 COLW 排会让输入框边界和列边界错开，看起来就是"乱套"。
        x = px
        cel = getattr(self, '_ce', None)
        for i in range(7):
            if cel and cel[2] == E_F0 + i:
                x += widths[i]
                continue                 # 正在原地编辑：位置由 cellBegin 自己管，别被布局挪走
            items.append((E_F0 + i, x, strip_y, widths[i] - 2, 26, on))
            x += widths[i]
        items += [(B_LOADROW, px, strip_y + 30, 72, 26, on),
                  (B_APPLYROW, px + 76, strip_y + 30, 72, 26, on),
                  (B_DELROW, px + 152, strip_y + 30, 72, 26, on)]
        # 表格：高度按"表头 + 整数行"对齐。不对齐的话最后一行只露半截，
        # 选中行会在编辑条上方留一条蓝边，看着像表格残留。
        tbl_y = ed_y + ED_HEAD_H + 2
        tbl_bottom = (strip_y - 8) if on else (foot_y - 10)
        rowH, hdr_h = 21, 24
        if hasattr(self.win, 'getTableMetrics'):
            try:
                m = self.win.get_table_metrics(TBL)
                if m and m[0] > 0:
                    rowH, hdr_h = int(m[0]), int(m[1])
            except Exception:
                pass
        body = max(rowH * 4, tbl_bottom - tbl_y - hdr_h)
        rows = max(4, body // rowH)
        tbl_h = hdr_h + rows * rowH
        self._tbl_pos = (px, tbl_y)          # 原地编辑按它把 EDIT 盖到单元格上
        items.append((TBL, px, tbl_y, inner, tbl_h, True))
        return items

    @staticmethod
    def gridColumns(inner):
        """按 HTML 的列宽比例铺满侧板（末列 ✕ 固定 26px）。"""
        fixed = 26
        n = len(FIELDS)
        base = float(sum(COLW[:n])) or 1.0
        avail = max(240, inner - 22 - fixed)     # 22 ≈ 垂直滚动条 + 边框
        widths = [max(30, int(round(COLW[i] * avail / base))) for i in range(n)]
        return widths + [fixed]

    def onSplitDrag(self, phase, x):
        """分割条拖动。x 是**分割条左边界在窗口客户区里的目标 x**（绝对位置）。

        这个数由控件自己解算：按下时记下自己的左边界与光标屏幕位置，之后每一帧
        只按屏幕位移推出"我现在该站哪"，并钳在父客户区内。上层拿到的就是手势要求的
        最终位置 —— 不做差分、不记基准，重排多少次都不会累积误差。

        侧板右贴窗口，所以 侧板宽 = 客户宽 - (分割条左边界 + 分割条宽)。
        """
        if phase not in ('move', 'up'):
            return
        # 合并重排：一次拖动能来上百个 move，每个都整窗重排 = 每帧 30~80ms。
        # 压到 ~30fps，松手那一下必须补最后一帧，否则停在旧位置。
        self._split_tip = int(x)
        now = time.perf_counter()
        # 时间窗内合并，但位移超过 6px 必须立刻应用：否则快速拖动会被合并成
        # "几乎没动"，测试与手感都表现为分割条跟不上手。
        moved = abs(int(x) - getattr(self, '_split_last', -99999))
        if (phase == 'move' and moved < 6
                and (now - getattr(self, '_split_t', 0.0)) < 0.033):
            return
        self._split_last = int(x)
        self._split_t = now
        cw, _ = self.client_size()
        lo, hi = self.sideBounds(cw)
        new_w = max(lo, min(hi, cw - int(x) - SPLIT_W))
        if new_w != self._ed_w:
            self._ed_w = new_w
            self.layout(drag=(phase == 'move'))

    def onResize(self, cw, ch):
        """宿主尺寸变化：按网格重算所有区块，并重算整图 fit 与状态栏。"""
        self.layout()
        if self.view:
            self.fitView()

    # ---------- 新旧表格统一访问层（旧 SysListView32 路径保留兼容） ----------
    def tblSel(self):
        return self.win.table_selected_row(TBL) if self._new_table else self.win.get_table_selected_row(TBL)

    def tblSetSel(self, r):
        if self._new_table:
            self.win.table_set_selected_row(TBL, r)
            if hasattr(self.win, 'tableEnsureVisible'):
                self.win.table_ensure_visible(TBL, r)
        else:
            self.win.set_table_selected_row(TBL, r)

    def tblGet(self, r, c):
        return self.win.table_get_cell(TBL, r, c) if self._new_table else self.win.get_table_cell(TBL, r, c)

    def tblSet(self, r, c, s):
        if self._new_table:
            self.win.table_set_cell(TBL, r, c, s)
        else:
            self.win.set_table_cell(TBL, r, c, s)

    def tblCellRect(self, r, c):
        return (self.win.table_cell_rect(TBL, r, c) if self._new_table
                else self.win.cell_rect(TBL, r, c))

    # ---------- 行编辑（双击表格某行） ----------
    def onGridDbl(self):
        """双击表格：直接在那一格里原地编辑（底部编辑条不再弹出来抢视线）。"""
        if getattr(self, '_ce', None):
            return                       # 宿主的 'dbl' 回调已经进入了原地编辑
        r = self.tblSel()
        if self.rowAt(r) is not None:
            self.cellBegin(r, 0)

    def openRowEdit(self, r):
        w = self.win
        if self.rowAt(r) is None:
            return
        self._row_edit_row = r
        self._row_edit_open = True
        for j in range(7):
            w.set_text(E_F0 + j, self.tblGet(r, j) or '')
        self.layout()
        ids = self.rowEditIds()
        for i in ids:
            w.set_visible(i, True)
        w.raise_many(ids)
        w.set_text(L_EDIT_COUNT, '正在编辑第 %d 行' % (r + 1))

    def rowEditClose(self):
        w = self.win
        was_open = getattr(self, '_row_edit_open', False)
        self._row_edit_open = False
        for i in self.rowEditIds():
            w.set_visible(i, False)
        if was_open:
            w.set_text(L_EDIT_COUNT, '共 %d 行' % len(self.tableRows()))

    def rowVals(self):
        return [(self.win.get_text(E_F0 + i) or '').strip() for i in range(7)]

    def onRowOk(self):
        r = getattr(self, '_row_edit_row', -1)
        if self.rowAt(r) is None:
            return
        err = self.applyRow(r, self.rowVals())
        if err:
            self.win.set_text(L_EDIT_COUNT, '第 %d 行：%s' % (r + 1, err))
            return
        self.rowEditClose()
        self.fillTable()
        self.tblSetSel(r)
        self.rebuild(fit=False)
        cur = getattr(self, 'curSel', '') or ''
        if cur and any(n.get('code') == cur for n in self.nodes):
            self.fillDetail(cur)
        self.win.set_text(L_EDIT_COUNT, '第 %d 行已更新' % (r + 1))

    def onRowDel(self):
        r = getattr(self, '_row_edit_row', -1)
        if self.rowAt(r) is None:
            return
        self.rowEditClose()
        self.gridDeleteRow(r)


    def onRowCancel(self):
        self.rowEditClose()

    # ---------- 原地单元格编辑（双击某格 = 直接在那一格里改） ----------
    def cellBegin(self, row, col):
        """双击单元格：把该列的 EDIT 精确盖到这一格上，聚焦并全选，就是 Excel 的手感。"""
        w = self.win
        n = len(self.tableRows())
        if not (0 <= row < n) or not (0 <= int(col) < 7):
            return
        col = int(col)
        if self._new_table and hasattr(w, 'tableEnsureVisible'):
            w.table_ensure_visible(TBL, row)
        rect_fn = getattr(w, 'tableCellRect', None) if self._new_table else getattr(w, 'cellRect', None)
        if not rect_fn:
            self.openRowEdit(row)          # 宿主不带该能力时退回底部编辑条
            return
        self.cellEnd(commit=True)           # 先把上一格结掉
        rc = rect_fn(TBL, row, col)
        if not rc:
            return
        cx, cy, cw, ch = rc
        tx, ty = self._tbl_pos
        eid = E_F0 + col
        self._ce = (row, col, eid)
        w.move_many([(eid, tx + cx, ty + cy, max(40, cw), max(20, ch), True)])
        w.set_text(eid, self.tblGet(row, col) or '')
        w.set_visible(eid, True)
        w.raise_many([eid])
        w.set_focus(eid)
        w.set_text(L_EDIT_COUNT, '正在编辑第 %d 行的「%s」' % (row + 1, COLS[col]))

    def cellBlur(self, *a):
        """EDIT 失焦 = 提交（点别处/点另一个格都算确认）。"""
        self.cellEnd(commit=True)

    def cellEnd(self, commit=True):
        ce = self._ce
        if not ce:
            return
        self._ce = None                       # 先清，避免 setVisible 触发失焦再进一遍
        row, col, eid = ce
        txt = (self.win.get_text(eid) or '').strip()
        self.win.set_visible(eid, False)
        if commit:
            self.cellApply(row, col, txt)

    def cellApply(self, row, col, txt):
        """单格写回：只改这一行的第 col 列，其余列沿用原值，再走 applyRow 落库。"""
        w = self.win
        rowD = self.rowAt(row)
        if rowD is None:
            return
        key = FIELDS[col]
        vals = self.rowsToValues([rowD])[0]
        vals[col] = txt
        err = self.applyRow(row, vals, edited=key)
        if err:
            w.set_text(L_EDIT_COUNT, '第 %d 行：%s' % (row + 1, err))
            return
        self.fillTable()                     # 双电源的行会一起变（编号/长度联动）
        self.tblSetSel(row)
        self.rebuild(fit=False)
        cur = getattr(self, 'curSel', '') or ''
        if cur and any(n.get('code') == cur for n in self.nodes):
            self.fillDetail(cur)
        w.set_text(L_EDIT_COUNT, '第 %d 行「%s」已更新' % (row + 1, COLS[col]))
    # ---------- 表格键盘 / 排序 / 粘贴 ----------
    def onGridAction(self, action, row, col, text):
        """宿主回调：F2=edit，Delete=delete，Insert=insert，Ctrl+V=paste，点列头=sort。"""

        n = len(self.tableRows())
        r = row if 0 <= row < n else self.tblSel()
        if action == 'cellclick':
            # 单击表格任何格：先把上一格的原地编辑提交掉（点别处=确认）
            if getattr(self, '_ce', None):
                self.cellEnd(commit=True)
        elif action == 'dbl':
            if 0 <= row < n:
                self.cellBegin(row, int(col))
        elif action == 'cellend':
            # row = 编辑框 id，col = 按键（13 回车确认 / 27 Esc 取消）
            self.cellEnd(commit=(int(col) != 27))
        elif action == 'edit':
            if 0 <= r < n:
                self.cellBegin(r, int(col or 0))
        elif action == 'delete':
            if 0 <= r < n:
                self.gridDeleteRow(r)
        elif action == 'insert':
            self.onAddRow()
        elif action == 'paste':
            self.gridPaste(text or '')
        elif action == 'sort':
            self.gridSort(int(col or 0))

    @staticmethod
    def num(text, default=0.0):
        try:
            return float(str(text).strip() or default)
        except (TypeError, ValueError):
            return default

    def gridPaste(self, text):
        """粘贴：按 tab / 换行拆分，逐行追加（编号重复或为空的行跳过）。"""
        rows = []
        for ln in text.replace('\r\n', '\n').replace('\r', '\n').split('\n'):
            if ln.strip():
                cells = [c.strip() for c in ln.split('\t')]
                cells += [''] * (len(FIELDS) - len(cells))
                rows.append(cells)
        if not rows:
            self.win.set_text(L_EDIT_COUNT, '剪贴板里没有可粘贴的行')
            return
        have = set(x['code'] for x in self.nodes)
        added = skipped = 0
        for cells in rows:
            code = cells[2]
            if not code or code in have:
                skipped += 1
                continue
            have.add(code)
            h, v, m = self.num(cells[6]), self.num(cells[7]), self.num(cells[8])
            self.nodes.append(dict(code=code, name='', upstream=cells[0],
                                   circuit=cells[1], cable=cells[3],
                                   loadKw=self.num(cells[10]), voltage=cells[4] or '380',
                                   strands=max(1, int(self.num(cells[5]) or 1)),
                                   hLen=h, vLen=v, margin=m,
                                   length=engine.calc_length(h, v, m)))
            added += 1
        self.fillTable()
        if added:
            self.rebuild(fit=False)
        self.win.set_text(L_EDIT_COUNT, '已粘贴 %d 行%s' % (added, ('，跳过 %d 行' % skipped) if skipped else ''))
        self.win.set_text(L_STATUS, '已粘贴 %d 行，共 %d 行' % (added, len(self.nodes)))

    def gridSort(self, col):
        """点列头排序：按该列重排 nodes（数值列按数值，其余按文本）。"""
        if not (0 <= col < len(FIELDS)):
            return
        key = FIELDS[col]
        if key in NUM_COLS:
            def k(n):
                return float(n.get(key) or 0)
        else:
            def k(n):
                return str(n.get(key) or '')
        self._sort_asc = not self._sort_asc if col == self._sort_col else True
        self._sort_col = col
        self.nodes = sorted(self.nodes, key=k, reverse=not self._sort_asc)
        self.fillTable()
        self.rebuild(fit=False)
        self.win.set_text(L_EDIT_COUNT, '按「%s」%s排序'
                         % (COLS[col], '升序' if self._sort_asc else '降序'))


        self.rowEditClose()

    def applyFonts(self):
        """统一字体：HTML 全部用 Microsoft YaHei（13px 常规 / 15px 标题 / 12px 状态栏）。"""
        if not hasattr(self.win, 'setControlFont'):
            return
        for cid, px, bold in self._font_ids:
            try:
                self.win.set_control_font(cid, 'Microsoft YaHei', px, bold)
            except Exception:
                pass

    def buildDetail(self):
        w = self.win
        self._detail_rc = {}

        def put(cid, x, y, ww, hh):
            self._detail_rc[cid] = (x, y, ww, hh)

        # HTML .detail: box-shadow 0 4px 12px rgba(16,24,40,.1)，radius 6、边框 #D0D5DD
        w.add_shadow_box(DT_PANEL, DT_X, DT_Y, DT_W, DT_H,
                       14, 6, 0, 4, 12, '#1A101828', '#FFFFFF', '#FFFFFF', '#D0D5DD', 1)
        put(DT_PANEL, DT_X, DT_Y, DT_W, DT_H)
        x0 = DT_X + 12
        inner = DT_W - 24
        self._detail_ids = [DT_PANEL, D_SAVE, D_ADDCHILD, D_DEL, DT_TITLE, DT_TIP]
        w.add_label(DT_TITLE, '节点', x0, DT_Y + 8, inner, 20)
        put(DT_TITLE, x0, DT_Y + 8, inner, 20)
        self._font_ids.append((DT_TITLE, 13, True))

        # 字段顺序与标签按用户口径：用电点 / 起点 / 回路 / 电缆型号，再 电压 / 功率 / 长度
        fields = [('用电点', D_F0 + 1), ('起点', D_F0 + 2), ('回路', D_F0 + 0), ('电缆型号', D_F0 + 3)]
        y = DT_Y + 34
        for i, (lb, fid) in enumerate(fields):
            w.add_label(DT_L0 + i, lb, x0, y, inner, 15)
            put(DT_L0 + i, x0, y, inner, 15)
            w.add_edit(fid, '', x0, y + 16, inner, 26)
            put(fid, x0, y + 16, inner, 26)
            self._detail_ids += [DT_L0 + i, fid]
            self._font_ids += [(DT_L0 + i, 12, False), (fid, 12, False)]
            y += 48

        # .ne-row：电压 / 功率 / 长度 三列并排（gap 8）
        w.add_label(DT_LV, '电压', x0, y, 104, 15)
        put(DT_LV, x0, y, 104, 15)
        w.add_combo(D_VOL, x0, y + 16, 104, 26)
        put(D_VOL, x0, y + 16, 104, 26)
        w.add_label(DT_LLOAD, '功率 kW', x0 + 112, y, 64, 15)
        put(DT_LLOAD, x0 + 112, y, 64, 15)
        w.add_edit(D_LOAD, '', x0 + 112, y + 16, 64, 26)
        put(D_LOAD, x0 + 112, y + 16, 64, 26)
        w.add_label(DT_LL, '长度 m', x0 + 184, y, 64, 15)
        put(DT_LL, x0 + 184, y, 64, 15)
        w.add_edit(D_LEN, '', x0 + 184, y + 16, 64, 26)
        put(D_LEN, x0 + 184, y + 16, 64, 26)
        for i in (DT_LV, DT_LL, DT_LLOAD):
            self._detail_ids.append(i)
            self._font_ids.append((i, 12, False))
        for i in (D_VOL, D_LEN, D_LOAD):
            self._detail_ids.append(i)
            self._font_ids.append((i, 12, False))
        y += 48

        # .ne-actions：保存并重绘（primary） / ＋下游节点 / 删除
        w.add_button(D_SAVE, '保存并重绘', x0, y + 4, 96, 26)
        put(D_SAVE, x0, y + 4, 96, 26)
        w.add_button(D_ADDCHILD, '＋下游节点', x0 + 104, y + 4, 80, 26)
        put(D_ADDCHILD, x0 + 104, y + 4, 80, 26)
        w.add_button(D_DEL, '删除', x0 + 192, y + 4, 52, 26)
        put(D_DEL, x0 + 192, y + 4, 52, 26)
        self._font_ids += [(D_SAVE, 12, False), (D_ADDCHILD, 12, False), (D_DEL, 12, False)]
        y += 36
        w.add_label(DT_TIP, '', x0, y, inner, 16)
        put(DT_TIP, x0, y, inner, 16)
        self._detail_ids.append(DT_TIP)
        self._font_ids.append((DT_TIP, 11, False))

        w.on_click(D_SAVE, self.onDetailSave)
        w.on_click(D_ADDCHILD, self.onDetailAddchild)
        w.on_click(D_DEL, self.onDetailDel)

    def buildEditor(self):
        """HTML 的 .editor 侧板。
        表格本体就是一个原生 ListView：自带列头、垂直滚动条、滚轮和整块绘制，
        不再用"每格一个 EDIT"去拼表格的样子。要改某一行时双击该行，
        弹出下面这条行编辑条（7 个输入 + 确定/删除/取消），确认后写回对应单元格。
        """
        w = self.win
        w.add_white_rect(ED_PANEL, ED_X, ED_Y, ED_W, ED_H)
        # 画布与侧板之间：6px 拖动热区（自身不着色）+ 1px 可见分割线。
        # 热区原来是 addWhiteRect：白底 + WS_BORDER，浅色背景上既看不到"线"，
        # 又能看到 1px 深框压在表格左边，看着就像表格画面越过了分割线。
        w.add_static_line(SPLIT, ED_X - SPLIT_W, ED_Y, SPLIT_W, ED_H)
        if hasattr(w, 'onDrag'):
            w.on_drag(SPLIT, self.onSplitDrag)
        # z 序：分割条必须压在表格/编辑框之上。子窗口是谁后建谁在上，
        # 而表格比分割条晚建 -> 表格边缘会盖住分割条，肉眼看就是"表格越过了分割线"。
        # 建完所有控件后统一把分割条提到最前（逐个 bringToFront 是 O(n²)，用 raiseMany）。
        w.raise_many([SPLIT])
        px = ED_X + ED_PAD
        self._font_ids += [(L_EDIT_TITLE, 13, False), (L_EDIT_COUNT, 13, False),
                           (L_EDIT_FOOT, 12, False)]
        w.add_label(L_EDIT_TITLE, '节点数据', px, ED_Y + 12, 120, 20)
        w.add_label(L_EDIT_COUNT, '', px + 78, ED_Y + 12, 100, 20)
        bx = ED_X + ED_W - ED_PAD - 72
        w.add_button(B_CLOSE, '关闭', bx, ED_Y + 8, 72, 26)
        w.add_button(B_APPLY, '应用并重绘', bx - 100, ED_Y + 8, 96, 26)
        w.add_button(B_ADDROW, '＋ 新增节点', bx - 196, ED_Y + 8, 92, 26)
        self._font_ids += [(B_CLOSE, 13, False), (B_APPLY, 13, False), (B_ADDROW, 13, False)]

        # 表格：新自绘表格 PyMfcTable（命中/选中/矩形同源）；宿主不带 addTable 时退回旧 ListView
        if self._new_table:
            w.add_table(TBL, px, ED_Y + ED_HEAD_H + 2, ED_W - 2 * ED_PAD, 300)
            w.set_table_columns(TBL, self.gridColumns(ED_W - 2 * ED_PAD), COLS)
            # code/name/upstream/cable/voltage = Text；loadKw/length = Double（右对齐 + 数值排序）
            # 11 列 + 删除列：name/code/upstream/circuit=Text，功率/拼数/水平/垂直/合计=Numeric
            # 起点/回路/终点/型号规格 = Text；根数/水平/垂直/余量/合计/功率 = Numeric；电压/删除 = Text
            w.set_table_col_types(TBL, [0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0])
        else:
            w.add_list_ctrl(TBL, px, ED_Y + ED_HEAD_H + 2, ED_W - 2 * ED_PAD, 300)
            w.set_control_font(TBL, 'Microsoft YaHei', 12, False)
            for i, name in enumerate(COLS):
                w.add_column(TBL, name, COLW[i])

        # 行编辑条（双击行才出现）
        for i in range(7):
            w.add_edit(E_F0 + i, '', px, ED_Y, 1, 1)
            self._font_ids.append((E_F0 + i, 12, False))
        w.add_button(B_LOADROW, '确定', px, ED_Y, 72, 26)
        w.add_button(B_APPLYROW, '删除', px + 76, ED_Y, 72, 26)
        w.add_button(B_DELROW, '取消', px + 152, ED_Y, 72, 26)
        self._font_ids += [(B_LOADROW, 12, False), (B_APPLYROW, 12, False), (B_DELROW, 12, False)]

        w.add_label(L_EDIT_FOOT, '上游填现有编号即挂到该节点下；留空为起点。双击某行改值，确定后点「应用并重绘」。',
                   px, ED_Y + ED_H - 30, ED_W - 2 * ED_PAD, 20)

        w.on_click(B_ADDROW, self.onAddRow)
        w.on_click(B_APPLY, self.onApplyAll)
        w.on_click(B_CLOSE, self.onCloseEditor)
        w.on_double_click(TBL, self.onGridDbl)
        if hasattr(w, 'onBlur'):
            for i in range(7):
                w.on_blur(E_F0 + i, self.cellBlur)
        w.on_click(B_LOADROW, self.onRowOk)
        w.on_click(B_APPLYROW, self.onRowDel)
        w.on_click(B_DELROW, self.onRowCancel)
        # 表格的键盘与列头动作（F2/Delete/Insert/Ctrl+C/V/点列头排序）在宿主里拦，
        # 动作回到这里处理 —— 数据源始终在 Python 这一侧。
        if hasattr(w, 'onGridAction'):
            w.on_grid_action(self.on_grid_action)

    def title(self):
        t = self.win.get_text(E_TITLE)
        return t.strip() if t else self.titleText

    def style(self):
        return M.powerplan.Style()

    def childrenMap(self):
        cm = {}
        for n in self.nodes:
            up = (n.get('upstream') or '').strip()
            if up:
                cm.setdefault(up, []).append(n['code'])
        return cm

    def descendants(self, code):
        return ENG.descendants(self.nodes, code)

    def layoutNodes(self):
        """剔除折叠掉的后代，并给折叠节点补 collapsedCount。"""
        hidden = set()
        for c in self.collapsed:
            hidden.update(self.descendants(c))
        view = []
        cm = self.childrenMap()
        for n in self.nodes:
            if n['code'] in hidden:
                continue
            item = dict(n)
            item['collapsedCount'] = len(self.descendants(n['code'])) if n['code'] in self.collapsed else 0
            view.append(item)
        return view

    def rebuild(self, fit=False):
        self._pending_rebuild = False
        win = self.win
        view_nodes = self.layoutNodes()
        # 坐标由规则引擎算（engine.layout），C++ 只按坐标画：屏幕不再调用 C++ 的排版
        scene = ENG.scene(view_nodes, self.horiz, self.title())
        self.view = dict(scene['layout'])
        self.view['nodes'] = scene['nodes']        # 状态栏要按节点算电压分档
        self.view['crossings'] = ENG.countCrossings(self.view['links'])
        sceneText = json.dumps(scene, ensure_ascii=False)
        self._scene_text = sceneText          # 导出复用同一份坐标，绝不重排
        win.load_dwg_scene(CV, sceneText)
        # 大图要把同一份 4.8 MB 场景再解析一遍给缩略图，约 90 ms；推到下一帧做
        if len(view_nodes) >= 3000:
            self._pendingMini = sceneText if MINI_VIA_JSON else True
        elif MINI_VIA_JSON:
            win.load_dwg_scene(MINI, sceneText)
            win.set_dwg_options(MINI, False, True, CV)
        else:
            win.copy_dwg_scene(MINI, CV)      # 拷主画布已解析好的场景，省一次 4.8 MB 解析
            win.set_dwg_options(MINI, False, True, CV)
        win.set_dwg_options(CV, self.showCable, False, -1)
        if fit:
            self.fitView()
        else:
            self.centerKeep()
        self.updateStatus()
        if self.curSel and any(n['code'] == self.curSel for n in self.view['nodes']):
            win.select_dwg(CV, self.curSel)
            self.fillDetail(self.curSel)
        else:
            self.curSel = ''
            self.fillDetail(None)
        # 一次提交：逐个 bringToFront 每次都要全表重排 z 序 + 整窗重画（4 次约 120 ms）
        win.bring_to_front_many([MI_PANEL, MINI, ST_PANEL, L_STATUS])
        if self.editing:
            # 侧板是画布内右侧的绝对定位层：必须压过状态栏，否则底栏文字会穿到侧板底
            win.bring_to_front_many(self.panelIds())
            self._detail_raised = False
        if self.curSel:
            self.showDetail()

    def countOverlaps(self):
        """HTML 状态栏里的"重叠"数：任意两张卡片矩形相交即计一对。"""
        return ENG.countOverlaps((self.view or {}).get('boxes') or [])

    def updateStatus(self):
        r = self.view
        if not r:
            return
        k = self.win.get_dwg_view(CV)[0]
        # 文案与统计全交给规则引擎（HTML 的 updateStatus 逐字一致由 rules.json 里的 status 段控制）
        self.win.set_text(L_STATUS, ENG.statusText(r, self.nodes, k, self.countOverlaps()))

    # ---------- 视图 ----------
    def viewArea(self):
        """绘图区 = 画布当前客户尺寸（layout 里已扣掉左右留白与右侧表格宽度）。
        必须读 self.cvW/cvH，不能读模块常量 CV_W/CV_H：那两个是启动时的尺寸，
        窗口最大化后画布实际变宽变高，用旧值算 fit 就会出现"视图不跟着缩放、
        像缩成全局"的现象。"""
        return max(200, self.cvW), max(120, self.cvH)

    def fitView(self):
        if not self.view:
            return
        W, H = self.view['width'], self.view['height']
        aw, ah = self.viewArea()
        # 与 C++ fitDwgCanvas 同一套口径：=1 地板 0.05/不封顶；=0 旧 HTML 口径 0.2/封顶 1.0。
        k = min((aw - 24) / W, (ah - 24) / H, PLAN_FIT_CAP)
        k = max(PLAN_ZOOM_MIN, min(PLAN_ZOOM_MAX, k))
        ox = int((aw - W * k) * 0.5)
        oy = int((ah - H * k) * 0.5)
        if not SURFACE_ON:
            ox = max(0, ox); oy = max(0, oy)
        self.win.set_dwg_view(CV, k, ox, oy)
        self.updateStatus()

    def zoomAt(self, factor):
        zoom, ox, oy = self.win.get_dwg_view(CV)
        if zoom <= 0:
            return
        nk = max(PLAN_ZOOM_MIN, min(PLAN_ZOOM_MAX, zoom * factor))
        cx, cy = self.cvW * 0.5, self.cvH * 0.5
        nox = int(cx - (cx - ox) * (nk / zoom))
        noy = int(cy - (cy - oy) * (nk / zoom))
        self.win.set_dwg_view(CV, nk, nox, noy)
        self.updateStatus()
        self.win.set_dwg_options(MINI, False, True, CV)

    def setZoom(self, k):
        zoom, ox, oy = self.win.get_dwg_view(CV)
        if zoom <= 0:
            return
        k = max(PLAN_ZOOM_MIN, min(PLAN_ZOOM_MAX, k))
        cx, cy = self.cvW * 0.5, self.cvH * 0.5
        nox = int(cx - (cx - ox) * (k / zoom))
        noy = int(cy - (cy - oy) * (k / zoom))
        self.win.set_dwg_view(CV, k, nox, noy)
        self.updateStatus()
        self.win.set_dwg_options(MINI, False, True, CV)

    def centerKeep(self):
        """布局重排后用同一个 zoom 重新居中（不跳到屏幕外）。"""
        if not self.view:
            return
        zoom = self.win.get_dwg_view(CV)[0] or 1.0
        W, H = self.view['width'], self.view['height']
        aw, ah = self.viewArea()
        ox = int((aw - W * zoom) * 0.5)
        oy = int((ah - H * zoom) * 0.5)
        self.win.set_dwg_view(CV, zoom, ox, oy)

    def centerOn(self, code):
        rc = self.win.card_rect_dwg(CV, code)
        if rc is None:
            return False
        zoom = self.win.get_dwg_view(CV)[0] or 1.0
        x, y, w, h = rc
        # cardRectDwg 是客户端矩形 → 反算到模型中心，再摆到视口中心
        _, ox, oy = self.win.get_dwg_view(CV)
        mx = (x - ox) / zoom + w / (2 * zoom)
        my = (y - oy) / zoom + h / (2 * zoom)
        aw, ah = self.viewArea()
        nox = int(aw * 0.5 - mx * zoom)
        noy = int(ah * 0.5 - my * zoom)
        self.win.set_dwg_view(CV, zoom, nox, noy)
        self.win.set_dwg_options(MINI, False, True, CV)
        return True

    # ---------- 交互 ----------
    def onTitleChange(self):
        if getattr(self, '_suppressTitleRebuild', False):
            return                    # 程序改标题不该触发整图重排
        self.rebuild(fit=False)

    def onDirChange(self):
        # HTML 的 dirSel：0 = 横向 左→右（h），1 = 纵向 上→下（v）
        self.horiz = (self.win.get_cur_sel(C_HORIZ) == 0)
        self.rebuild(fit=True)

    def onCableToggle(self):
        self.showCable = bool(self.win.get_check(C_CABLE))
        self.win.set_dwg_options(CV, self.showCable, False, -1)

    def onCanvasClick(self):
        code = self.win.get_dwg_selection(CV)
        self.curSel = code
        self.fillDetail(code)

    def onCanvasDblclick(self):
        code = self.win.get_dwg_selection(CV)
        if not code:
            return
        if not self.descendants(code):
            return                    # 没有下游，折叠没有意义
        if code in self.collapsed:
            self.collapsed.discard(code)
        else:
            self.collapsed.add(code)
        self.curSel = code
        self.rebuild(fit=False)

    def onMiniClick(self):
        # 跳跃已由 C++ 画布自己做（点哪就把主画布居中到那一点），这里只刷新状态栏
        self.updateStatus()

    def centerSelectedOrRoot(self, zoom):
        code = self.curSel
        if not code and self.view and self.view['nodes']:
            code = self.view['nodes'][0]['code']
        if code:
            self.centerOn(code)

    def onSearchChange(self):
        # HTML 是 #search 的 input 事件：输入即搜，不用按按钮
        self.doSearch()

    def checkHotkeys(self):
        """画布/表格上按 Ctrl+F：把焦点送到搜索框并清空，直接输入编号或名称即定位。
        用 GetAsyncKeyState 轮询而不改 C++ 消息链，画布焦点在谁身上都能生效。"""
        u = ctypes.windll.user32
        down = bool(u.GetAsyncKeyState(0x11) & 0x8000) and bool(u.GetAsyncKeyState(0x46) & 0x8000)
        if down and not getattr(self, '_fDown', False):
            self.win.set_text(E_SEARCH, '')
            self.win.set_focus(E_SEARCH)
            self.win.set_text(L_STATUS, 'Ctrl+F：输入编号或名称，敲字即定位节点')
        self._fDown = down

    def doSearch(self):
        q = (self.win.get_text(E_SEARCH) or '').strip()
        if not q:
            return
        hit = None
        for n in self.view['nodes'] if self.view else []:
            if q.lower() in (n['code'] or '').lower() or q in (n['name'] or ''):
                hit = n['code']
                break
        if not hit:
            self.win.set_text(L_STATUS, '没有匹配 "%s" 的节点' % q)
            return
        # 命中的节点若被折叠，先展开其祖先
        cm = self.childrenMap()
        anc, cur = [], hit
        while True:
            up = next((n.get('upstream') or '' for n in self.nodes if n['code'] == cur), '')
            if not up:
                break
            anc.append(up)
            cur = up
        changed = False
        for a in anc:
            if a in self.collapsed:
                self.collapsed.discard(a)
                changed = True
        if changed:
            self.rebuild(fit=False)
        self.curSel = hit
        self.win.select_dwg(CV, hit)
        self.centerOn(hit)
        self.fillDetail(hit)
        self.updateStatus()

    # ---------- 详情 ----------
    def nodeOf(self, code):
        for n in self.nodes:
            if n['code'] == code:
                return n
        return None

    def depthOf(self, code):
        return ENG.depthOf(self.nodes, code)

    def hideDetail(self):
        self._detail_visible = False
        # 只隐藏控件，不撤销原生 z 序：详情控件置顶关系在隐藏期间仍然有效，
        # 再次弹出时不必重跑 raiseMany / RDW_ALLCHILDREN。编辑器开合会显式重置该标记。
        for i in self._detail_ids:
            self.win.set_visible(i, False)

    def showDetail(self):
        # layout 里 detail 那几条已经带 show 标志，位置和显隐一次做完；
        # 原来这里 layout() 之后再 positionDetail() 又调用一次 layout()，
        # 等于每次点节点都整窗重排两遍，这就是"属性弹窗慢"的由来。
        # 热态：详情已经可见、z 序也已置顶时，位置不会再变，直接早退，省掉这一遍整窗 layout。
        if self._detail_visible and self._detail_raised:
            return
        self._detail_visible = True
        self.layout()
        # z 序只在第一次置顶、或侧板/画布层级变化后需要重排；重复点节点时
        # 详情浮层的兄弟关系没变，再跑一次 raiseMany 的 64 个 SetWindowPos 是白付。
        if self._detail_raised:
            return
        self._detail_raised = True
        ids = self._detail_ids
        if hasattr(self.win, 'raiseMany'):
            self.win.raise_many(ids)    # 一次批量置顶，避免 O(n²) 逐个 bringToFront
        else:
            for i in ids:
                self.win.bring_to_front(i)

    def positionDetail(self):
        """HTML positionDetail：详情浮层固定在画布右上角，位置由 layout 统一算
        （侧板打开时画布变窄，浮层跟着左移，等价于 HTML 里 right:12px 的吸附）。"""
        self.layout()

    def fillDetail(self, code):
        """HTML renderNodeForm：h3 + 4 个 label/input + 电压/长度/负载 + 3 按钮 + tip。"""
        n = self.nodeOf(code) if code else None
        if not n:
            self.hideDetail()
            return
        win = self.win
        win.set_text(DT_TITLE, '节点 %s' % n['code'])
        win.set_text(D_F0 + 0, n['code'])
        win.set_text(D_F0 + 1, n.get('name') or '')
        win.set_text(D_F0 + 2, n.get('upstream') or '')
        win.set_text(D_F0 + 3, n.get('cable') or '')

        # 电压下拉：选项是固定的 5 档，建控件时只灌一次；每次点节点只改选中项。
        # （CB_ADDSTRING 每条约 5ms，逐节点重建下拉串会白白拖慢详情弹出。）
        cur = normVolt(n.get('voltage'))
        opts = [('380', '380V 三相'), ('220', '220V 单相'), ('10kV', '10kV 中压'),
                ('35kV', '35kV 中压'), ('110kV', '110kV 高压')]
        if cur and all(o[0] != cur for o in opts):
            opts.append((cur, voltText(cur)))
        keys = tuple(o[0] for o in opts)
        if keys != self._vol_opts_seen:
            win.clear_strings(D_VOL)
            for o in opts:
                win.add_string(D_VOL, o[1])
            self._vol_opts_seen = keys
        win.set_cur_sel(D_VOL, next((i for i, o in enumerate(opts) if o[0] == cur), 0))

        vlen = float(n.get('length') or 0)
        vload = float(n.get('loadKw') or 0)
        win.set_text(D_LEN, ('%g' % vlen) if vlen > 0 else '')
        win.set_text(D_LOAD, ('%g' % vload) if vload > 0 else '')

        down = len(self.descendants(n['code']))
        tip = '第 %d 层 · 下游 %d 个' % (self.depthOf(n['code']) + 1, down)
        if n['code'] in self.collapsed:
            tip += '（已折叠）'
        win.set_text(DT_TIP, tip)
        self.showDetail()

    # ---------- 详情里的节点编辑（照 HTML 的 saveNodeEdit / addChildNode / deleteNode）----------
    def detailValues(self):
        code = (self.win.get_text(D_F0 + 0) or '').strip()
        name = (self.win.get_text(D_F0 + 1) or '').strip()
        upstream = (self.win.get_text(D_F0 + 2) or '').strip()
        cable = (self.win.get_text(D_F0 + 3) or '').strip()
        sel = self.win.get_cur_sel(D_VOL)
        volt_label = self.win.get_string(D_VOL, sel) if sel >= 0 else '380V 三相'
        volt_key = '380'
        if volt_label == '220V 单相':
            volt_key = '220'
        elif volt_label and volt_label != '380V 三相':
            volt_key = volt_label.split(' ')[0]
        def num(s):
            try:
                return float(s)
            except ValueError:
                return 0.0
        return dict(code=code, name=name, upstream=upstream, cable=cable,
                    voltage=volt_key,
                    length=num((self.win.get_text(D_LEN) or '').strip()),
                    loadKw=num((self.win.get_text(D_LOAD) or '').strip()))

    def onDetailSave(self):
        if not self.curSel:
            return
        n = self.nodeOf(self.curSel)
        if not n:
            return
        nd = self.detailValues()
        if not nd['code']:
            return
        if nd['code'] != self.curSel and any(x['code'] == nd['code'] for x in self.nodes):
            return
        old = self.curSel
        n.update(nd)
        if old != nd['code']:
            for x in self.nodes:
                if (x.get('upstream') or '') == old:
                    x['upstream'] = nd['code']
            if old in self.collapsed:
                self.collapsed.discard(old)
                self.collapsed.add(nd['code'])
        self.curSel = nd['code']
        self.fillTable()
        self.rebuild(fit=False)
        self.win.select_dwg(CV, nd['code'])
        self.fillDetail(nd['code'])

    def onDetailAddchild(self):
        if not self.curSel:
            return
        parent = self.nodeOf(self.curSel)
        if not parent:
            return
        codes = set(x['code'] for x in self.nodes)
        i = 1
        while ('%s-%d' % (self.curSel, i)) in codes:
            i += 1
        code = '%s-%d' % (self.curSel, i)
        self.nodes.append(dict(code=code, name='', upstream=self.curSel, cable='',
                               loadKw=0.0, voltage=parent.get('voltage') or '380',
                               length=0.0))
        self.curSel = code
        self.fillTable()
        self.rebuild(fit=False)
        self.win.select_dwg(CV, code)
        self.fillDetail(code)

    def onDetailDel(self):
        if not self.curSel:
            return
        n = self.nodeOf(self.curSel)
        if not n:
            return
        code = self.curSel
        self.nodes = [x for x in self.nodes if x['code'] != code]
        for x in self.nodes:                       # 下游上移一层
            if (x.get('upstream') or '') == code:
                x['upstream'] = n.get('upstream') or ''
        self.collapsed.discard(code)
        up = n.get('upstream') or ''
        self.curSel = up if up else ''
        self.fillTable()
        self.rebuild(fit=False)
        if up:
            self.win.select_dwg(CV, up)
            self.fillDetail(up)
        else:
            self.win.select_dwg(CV, '')
            self.hideDetail()

    # ---------- 示例 ----------
    def loadNodes(self, nodes, title, rawRows=None):
        self.nodes = nodes
        self._rawRows = rawRows          # 导入时直接用 CSV 行文本灌表，省一次 nodes→rows 转换
        self.collapsed.clear()
        self.curSel = ''
        # E_TITLE 挂 onChange→rebuild：程序改标题会同步触发一次整图重排（10221 节点约 0.5 s）
        self._suppressTitleRebuild = True
        try:
            self.win.set_text(E_TITLE, title)
            # 标题框是节点化自绘：抑制 onChange 后没有 rebuild 帮它重画，这里单独失效一次
            self.win.repaint(E_TITLE)
        finally:
            self._suppressTitleRebuild = False
        if self.editing:
            self.fillTable()
        # 上万节点：画布重排（约 0.6 s）推到主循环下一帧做，先把表格刷出来，避免一次性冻住
        if len(nodes) >= 2000:
            self._pending_rebuild = True
        else:
            self.rebuild(fit=True)

    def onSampleDoc(self):
        # 「文档示例」改为超大数据填充（20×10×50，共 10221 行），用于观察大表/大图表现
        txt = sampleBigCsv()
        self.importCsvText(txt, '文档示例 — 超大数据 %d 行' % (txt.count('\n') - 1))

    def onThreeLevel(self):
        self.loadNodes(sampleThreeLevel(), '三级配电干线图')

    def onDualSource(self):
        self.loadNodes(sampleDual(), '双电源配电干线图')

    # ---------- 导入 ----------
    def onImportCsv(self):
        p = fileDialog(False, '导入 CSV', 'CSV 文件 (*.csv)|*.csv|所有文件 (*.*)|*.*', owner=self.win.hwnd)
        if not p:
            return
        self.importCsvText(readText(p), os.path.basename(p))

    @staticmethod
    def parseCsvText(text):
        """计算书 CSV 文本 → FIELDS 顺序的行。表头认计算书列名（起点/回路/终点/
        型号规格/电缆根数/水平/垂直/余量/合计），也认旧的 7 列口径。"""
        txt = (text or '').lstrip('\ufeff').replace('\r\n', '\n').replace('\r', '\n')
        rdr = [r for r in csv.reader(io.StringIO(txt)) if any((c or '').strip() for c in r)]
        if not rdr:
            return []
        head = [(c or '').strip() for c in rdr[0]]
        aliases = {
            'upstream': ('起点', '取电点', '上游'),
            'circuit': ('回路', '回路编号'),
            'code': ('终点', '设备编号', '编号'),
            'cable': ('型号规格', '电缆型号', '电缆'),
            'strands': ('电缆根数', '根数', '电缆拼数', '拼数'),
            'hLen': ('水平长度',),
            'vLen': ('垂直长度',),
            'margin': ('余量',),
            'length': ('合计', '合计长度', '长度m', '长度'),
            'loadKw': ('功率', '功率kW', '负载kW', '负载'),
            'voltage': ('电压等级', '电压'),
        }
        idx = {}
        for j, k in enumerate(FIELDS):
            for i, h in enumerate(head):
                h2 = h.split('(')[0].split('（')[0].strip()
                if h2 in aliases[k] or h in aliases[k]:
                    idx[j] = i
                    break
        if len(idx) < 4:
            rows = rdr                     # 没表头：整体按固定列序当数据读
        else:
            rows = [[(r[idx[j]] if j in idx and idx[j] < len(r) else '').strip()
                     for j in range(len(FIELDS))] for r in rdr[1:]]
        return [r for r in rows if (r[2] or '').strip()]
    def importCsvText(self, text, title):
        try:
            rows = self.parseCsvText(text)
            nodes = self.nodesFromRows(rows)
        except Exception as e:
            self.win.set_text(L_STATUS, '导入失败：%s' % e)
            return
        if not nodes:
            self.win.set_text(L_STATUS, '导入失败：文件里没有有效数据行')
            return
        self.loadNodes(nodes, title, rawRows=rows)

    def onImportXlsx(self):
        p = fileDialog(False, '导入 XLSX', 'Excel 工作簿 (*.xlsx)|*.xlsx|所有文件 (*.*)|*.*', owner=self.win.hwnd)
        if not p:
            return
        try:
            rows = readXlsx(p)
        except Exception as e:
            self.win.set_text(L_STATUS, 'XLSX 解析失败：%s' % e)
            return
        if not rows:
            self.win.set_text(L_STATUS, 'XLSX 里没有可读的行')
            return
        try:
            nodes = self.nodesFromRows(self.parseCsvText(rowsToCsv(rows)))
        except Exception as e:
            self.win.set_text(L_STATUS, 'XLSX 导入失败：%s' % e)
            return
        if not nodes:
            self.win.set_text(L_STATUS, 'XLSX 里没有有效的设备行')
            return
        self.loadNodes(nodes, os.path.basename(p))
    def onImportJson(self):
        p = fileDialog(False, '导入 JSON', 'JSON 文件 (*.json)|*.json|所有文件 (*.*)|*.*', owner=self.win.hwnd)
        if not p:
            return
        self.importJsonText(readText(p), os.path.basename(p))

    def importJsonText(self, text, title):
        try:
            r = M.powerplan.parseJson(text)
        except Exception as e:
            self.win.set_text(L_STATUS, '导入失败：%s' % e)
            return
        self.loadNodes(r['nodes'], title)

    def onPasteJson(self):
        txt = clipboardText()
        if not txt.strip():
            self.win.set_text(L_STATUS, '剪贴板里没有文本')
            return
        self.importJsonText(txt, '粘贴的 JSON')

    # ---------- 导出 ----------
    def sceneText(self):
        """导出用的场景 JSON：与屏幕上是同一份（engine.layout 出的坐标）。"""
        text = getattr(self, '_scene_text', '') or ''
        if not text:
            scene = ENG.scene(self.layoutNodes(), self.horiz, self.title())
            text = json.dumps(scene, ensure_ascii=False)
            self._scene_text = text
        return text

    def sceneView(self):
        """导出 json/csv/svg：吃场景、只按坐标出，不再让 C++ 重新排版。
        之前走 powerplan.build 会带自己的纸面边距和标题块，导出尺寸与屏上对不上
        （屏上 6174×428，导出 6174×530）。"""
        return M.powerplan.fromScene(self.sceneText(), self.title(), False)

    def onExportJson(self):
        if not self.view:
            return
        p = fileDialog(True, '导出 JSON', 'JSON 文件 (*.json)|*.json', 'power-plan.json', self.win.hwnd)
        if not p:
            return
        writeText(p, self.sceneView()['json'])
        self.win.set_text(L_STATUS, '已导出 JSON：%s' % p)

    def onExportCsv(self):
        if not self.view:
            return
        p = fileDialog(True, '导出 CSV', 'CSV 文件 (*.csv)|*.csv', 'power-plan.csv', self.win.hwnd)
        if not p:
            return
        writeText(p, self.csvText(), bom=True)

    def onExportXlsx(self):
        if not self.view:
            return
        p = fileDialog(True, '导出 XLSX', 'Excel 工作簿 (*.xlsx)|*.xlsx', 'power-plan.xlsx', self.win.hwnd)
        if not p:
            return
        try:
            rows = list(csv.reader(io.StringIO(self.csvText())))
        except Exception as e:
            self.win.set_text(L_STATUS, '导出 XLSX 失败：%s' % e)
            return
        self.win.set_text(L_STATUS, '已导出 XLSX：%s' % p)

    def onExportSvg(self):
        if not self.view:
            return
        p = fileDialog(True, '导出 SVG', 'SVG 矢量图 (*.svg)|*.svg', 'power-plan.svg', self.win.hwnd)
        if not p:
            return
        writeText(p, self.sceneView()['svg'])
        self.win.set_text(L_STATUS, '已导出 SVG：%s' % p)

    def onExportPng(self):
        if not self.view:
            return
        p = fileDialog(True, '导出 PNG（2 倍）', 'PNG 图片 (*.png)|*.png', 'power-plan.png', self.win.hwnd)
        if not p:
            return
        try:
            size = M.powerplan.renderScenePng(p, self.sceneText(), 2)
        except Exception as e:
            self.win.set_text(L_STATUS, '导出 PNG 失败：%s' % e)
            return
        self.win.set_text(L_STATUS, '已导出 PNG（2x %dx%d）：%s' % (size[0], size[1], p))

    # ---------- 编辑侧板 ----------
    def panelIds(self):
        """侧板常显件（不含行编辑条）。"""
        return [SPLIT, ED_PANEL, L_EDIT_TITLE, L_EDIT_COUNT, B_ADDROW, B_APPLY, B_CLOSE,
                L_EDIT_FOOT, TBL]

    def rowEditIds(self):
        return [E_F0 + i for i in range(7)] + [B_LOADROW, B_APPLYROW, B_DELROW]

    def showEditor(self):
        """HTML 的 .editor：贴右 640 宽。画布让出这块宽度，其余布局由 layout 统一算。"""
        self.editing = True
        self._row_edit_open = False
        self.fillTable()
        self.layout()
        for i in self.panelIds():
            self.win.set_visible(i, True)
        for i in self.rowEditIds():
            self.win.set_visible(i, False)
        self.win.raise_many(self.panelIds())
        self._detail_raised = False
        self.win.set_text(L_EDIT_COUNT, '共 %d 行' % len(self.tableRows()))
        self.fitView()

    show_editor = showEditor

    def hideEditor(self):
        self.editing = False
        self._row_edit_open = False
        for i in self.panelIds() + self.rowEditIds():
            self.win.set_visible(i, False)
        self.layout()
        if self.view:
            self.fitView()
        self.win.bring_to_front(CV)
        self.win.bring_to_front(MI_PANEL)
        self.win.bring_to_front(MINI)
        self._detail_raised = False
        self.win.bring_to_front(ST_PANEL)
        self.win.bring_to_front(L_STATUS)
        self.positionDetail()
        if self.curSel:
            self.showDetail()

    def onOpenEditor(self):
        self.showEditor()

    def onToggleEditor(self):
        # HTML 的 btnEdit：开着就关，关着就开
        if self.editing:
            self.hideEditor()
        else:
            self.showEditor()

    def onCloseEditor(self):
        self.hideEditor()

    def gridText(self, row, key):
        """一格显示文案：长度按计算书口径（水平/垂直 3 位小数，合计取整）。"""
        if key == 'voltage':
            # 计算书里这一列写的是电压值本身（380V / 220V / 10kV），不带相别后缀
            return ENG.volt_style(row.get('voltage')).get('text') or ENG.normVoltage(row.get('voltage'))
        v = row.get(key)
        if key == 'strands':
            return '%d' % int(float(v or 1))
        if key == 'length':
            return '%d' % int(round(float(v or 0)))
        if key in ('hLen', 'vLen', 'margin', 'loadKw'):
            return '%g' % round(float(v or 0), 3)
        return str(v or '')

    # ---------- 表格行 = 一条进线一行（对齐用户的电缆计算书） ----------
    def tableRows(self):
        """一行 = 一条进线（一根电缆）。双电源柜两条进线 → 两行：
        终点（设备编号）相同，起点、回路、型号规格、根数与各段长度各填各的。"""
        out = []
        for n in self.nodes:
            for i, f in enumerate(engine.feeds_of(n)):
                out.append({'n': n, 'feed': i,
                            'upstream': f['upstream'], 'circuit': f['circuit'],
                            'code': n.get('code') or '',
                            'cable': f['cable'], 'strands': f['strands'],
                            'hLen': f['hLen'], 'vLen': f['vLen'],
                            'margin': f['margin'], 'length': f['length'],
                            'loadKw': float(n.get('loadKw') or 0),
                            'voltage': n.get('voltage') or '',
                            'name': n.get('name') or ''})
        return out

    def rowAt(self, r):
        rows = self.tableRows()
        return rows[r] if 0 <= r < len(rows) else None

    @staticmethod
    def syncFeeds(n):
        """把节点的 feeds 与节点级字段对齐：upstream 用 | 拼多值，
        型号规格/回路/根数/各段长度取第一条进线（画布卡片与连线标注用它们）。"""
        fs = n.get('feeds') or []
        if not fs:
            return
        n['upstream'] = '|'.join((f.get('upstream') or '') for f in fs
                                 if (f.get('upstream') or '') != '')
        n['circuit'] = fs[0].get('circuit') or ''
        n['cable'] = fs[0].get('cable') or ''
        n['strands'] = int(float(fs[0].get('strands') or 1))
        n['hLen'] = float(fs[0].get('hLen') or 0)
        n['vLen'] = float(fs[0].get('vLen') or 0)
        n['margin'] = float(fs[0].get('margin') or 0)
        n['length'] = float(fs[0].get('length') or 0)

    def applyRow(self, r, vals, edited=''):
        """把表格一行的 11 列写回节点/进线；返回错误文案（成功返回 ''）。

        合计 = ceil(水平 + 垂直 + 余量)，始终重算；直接改合计时反推余量。
        """
        row = self.rowAt(r)
        if row is None:
            return ''
        n, fi = row['n'], row['feed']
        v = [str(x or '').strip() for x in (list(vals) + [''] * len(FIELDS))[:len(FIELDS)]]
        up, circuit, code, cable, volt, strands, h, vv, margin, length, load = v
        if not code:
            return '终点不能为空'
        volt = volt or '380'
        if strands and float(strands or 0) < 1:
            return '电缆根数必须是不小于 1 的整数'
        for lab, s in (('电缆根数', strands), ('水平长度', h), ('垂直长度', vv),
                       ('余量', margin), ('合计', length), ('功率', load)):
            try:
                fv = float(s or 0)
            except ValueError:
                return '%s必须是不小于 0 的数字' % lab
            if fv < 0:
                return '%s必须是不小于 0 的数字' % lab
        if up and up == code:
            return '起点不能是终点自己'
        if code != (n.get('code') or '') and any(
                o.get('code') == code and o is not n for o in self.nodes):
            return '终点（%s）已存在' % code

        hf = round(float(h or 0), 3)
        vf = round(float(vv or 0), 3)
        mf = round(float(margin or 0), 3)
        if edited == 'length':
            # 直接改合计：按"水平 + 垂直 + 余量 = 合计"反推余量
            mf = max(0.0, round(float(length or 0) - hf - vf, 3))
        lf = engine.calc_length(hf, vf, mf)

        fs = engine.feeds_of(n)
        while len(fs) <= fi:
            fs.append({'upstream': '', 'circuit': '', 'cable': '', 'strands': 1,
                       'hLen': 0, 'vLen': 0, 'margin': 0, 'length': 0})
        f = fs[fi]
        f['upstream'] = up
        f['circuit'] = circuit
        f['cable'] = cable
        f['strands'] = int(float(strands or 1))
        f['hLen'], f['vLen'], f['margin'], f['length'] = hf, vf, mf, lf
        n['feeds'] = fs
        n['loadKw'] = float(load or 0)
        n['voltage'] = ENG.normVoltage(volt)
        old_code = n.get('code') or ''
        if code != old_code:
            for o in self.nodes:
                if (o.get('upstream') or '') == old_code:
                    o['upstream'] = code
                for ff in (o.get('feeds') or []):
                    if (ff.get('upstream') or '') == old_code:
                        ff['upstream'] = code
            self.collapsed.discard(old_code)
            if self.curSel == old_code:
                self.curSel = code
        n['code'] = code
        self.syncFeeds(n)
        return ''

    @staticmethod
    def nodesFromRows(rows):
        """计算书行（FIELDS 顺序）→ 节点。同终点（编号）多行 = 多进线，并成 feeds。"""
        nodes, byCode = [], {}
        for vals in rows:
            v = [str(x or '').strip() for x in (list(vals) + [''] * len(FIELDS))[:len(FIELDS)]]
            up, circuit, code, cable, volt, strands, h, vv, margin, length, load = v
            if not code:
                continue
            def _f(s):
                try:
                    return float(s or 0)
                except ValueError:
                    return 0.0
            hf, vf, mf, lf = _f(h), _f(vv), _f(margin), _f(length)
            if lf and not hf and not vf and not mf:
                hf = lf                      # 老数据只有合计 → 记作水平长度
            try:
                strands_i = max(1, int(float(strands or 1)))
            except ValueError:
                strands_i = 1
            feed = {'upstream': up, 'circuit': circuit, 'cable': cable,
                    'strands': strands_i, 'hLen': hf, 'vLen': vf, 'margin': mf,
                    'length': engine.calc_length(hf, vf, mf) if (hf or vf or mf) else lf}
            n = byCode.get(code)
            if n is None:
                n = dict(code=code, name='', upstream='', cable='', loadKw=_f(load),
                         voltage=ENG.normVoltage(volt), length=0, feeds=[])
                byCode[code] = n
                nodes.append(n)
            n['feeds'].append(feed)
        for n in nodes:
            App.syncFeeds(n)
        return nodes

    @staticmethod
    def rowsToValues(rows):
        """行dict → 按 FIELDS 顺序的 11 个显示值（表格/CSV/XLSX 共用）。"""
        out = []
        for r in rows:
            vals = []
            for k in FIELDS:
                if k == 'strands':
                    vals.append('%d' % int(float(r.get(k) or 1)))
                elif k == 'length':
                    vals.append('%d' % int(round(float(r.get(k) or 0))))
                elif k in ('hLen', 'vLen', 'margin', 'loadKw'):
                    vals.append('%g' % round(float(r.get(k) or 0), 3))
                elif k == 'voltage':
                    vals.append(ENG.voltText(r.get(k)))
                else:
                    vals.append(str(r.get(k) or ''))
            out.append(vals)
        return out

    def csvText(self):
        """导出计算书 CSV：列与用户计算书一比一（起点/回路/终点/型号规格/
        电缆根数/水平长度/垂直长度/余量/合计），双电源柜两行。"""
        buf = io.StringIO()
        wr = csv.writer(buf, lineterminator='\r\n')
        wr.writerow(COLS[:BOOK_N])
        for vals in self.rowsToValues(self.tableRows()):
            wr.writerow(vals[:BOOK_N])
        return buf.getvalue()
    def fillTable(self):
        """一次灌满整表（setTableRows：行间 \n、格间 \t）。

        逐行 appendTableRow 每插一行都重绘一次（39 行实测 22.5 ms 且卡顿），
        整表一次提交只在最后失效一遍；新自绘表格与旧 ListView 都走这条。
        """
        w = self.win
        raw = getattr(self, '_rawRows', None)
        self._rawRows = None
        if raw and self._new_table and len(raw) == len(self.nodes):
            # 一次导入：CSV 行本身就是表格要显示的文本，直接拼串（10221 行省掉 11 万次 gridText）
            w.set_table_rows(TBL, u'\n'.join(u'\t'.join([(c or '') for c in r]) for r in raw))
            self._grid_rows = len(raw)
            w.set_text(L_EDIT_COUNT, '共 %d 行' % len(raw))
            return
        rows = self.tableRows()
        lines = []
        for row in rows:
            cells = [self.gridText(row, k) for k in FIELDS]
            if not self._new_table:
                cells.append(u'\u2715')
            lines.append(u'\t'.join(cells))
        w.set_table_rows(TBL, u'\n'.join(lines))
        self._grid_rows = len(rows)
        w.set_text(L_EDIT_COUNT, '共 %d 行' % len(rows))
    def collectGrid(self):
        """从表格读回全部行（FIELDS 顺序）；同终点多行并进 feeds。"""
        w = self.win
        nrows = len(self.tableRows()) if self._new_table else w.get_table_row_count(TBL)
        rows = []
        for i in range(nrows):
            vals = [(self.tblGet(i, j) or '').strip() for j in range(len(FIELDS))]
            if not vals[2]:
                return None, '第 %d 行：终点不能为空' % (i + 1)
            for idx, lab in ((5, '电缆根数'), (6, '水平长度'), (7, '垂直长度'),
                             (8, '余量'), (9, '合计'), (10, '功率')):
                try:
                    f = float(vals[idx] or 0)
                except ValueError:
                    return None, '第 %d 行：%s必须是不小于 0 的数字' % (i + 1, lab)
                if f < 0:
                    return None, '第 %d 行：%s必须是不小于 0 的数字' % (i + 1, lab)
            if vals[0] and vals[0] == vals[2]:
                return None, '第 %d 行：起点不能是终点自己' % (i + 1)
            rows.append(vals)
        return self.nodesFromRows(rows), ''
    def gridDeleteRow(self, r):
        """删表格一行：双电源柜删的是那一条进线，其余行（含另一进线）保留。"""
        row = self.rowAt(r)
        if row is None:
            return
        n, fi = row['n'], row['feed']
        fs = engine.feeds_of(n)
        if fi > 0 and len(fs) > 1:
            code = n.get('code')
            del fs[fi]
            n['feeds'] = fs
            self.syncFeeds(n)
            self.fillTable()
            self.rebuild(fit=False)
            self.win.set_text(L_EDIT_COUNT, '已删除 %s 的第 %d 条进线' % (code, fi + 1))
            return
        self.onGridDelete(n.get('code'))
    def onGridDelete(self, code):
        if self.descendants(code):
            self.win.set_text(L_EDIT_COUNT, '节点 %s 还有下游，先删下游' % code)
            return
        self.nodes = [n for n in self.nodes if n['code'] != code]
        self.collapsed.discard(code)
        if self.curSel == code:
            self.curSel = ''
        self.fillTable()
        self.rebuild(fit=False)
        self.win.set_text(L_EDIT_COUNT, '已删除 %s' % code)

    def readForm(self):
        vals = [self.win.get_text(E_F0 + i) or '' for i in range(7)]
        out = dict(code=vals[0].strip(), name=vals[1].strip(), upstream=vals[2].strip(),
                   cable=vals[3].strip())
        try:
            out['loadKw'] = float(vals[4].strip() or 0)
        except ValueError:
            out['loadKw'] = 0.0
        out['voltage'] = vals[5].strip() or '380'
        try:
            out['length'] = float(vals[6].strip() or 0)
        except ValueError:
            out['length'] = 0.0
        return out

    def onAddRow(self):
        """新增一台设备（默认起点、单条进线），选中并在第一格进入原地编辑。"""
        codes = set(n['code'] for n in self.nodes)
        i = 1
        while ('N%d' % i) in codes:
            i += 1
        code = 'N%d' % i
        self.nodes.append(dict(code=code, name='', upstream='', circuit='',
                               cable='', loadKw=0.0, voltage='380', strands=1,
                               hLen=0.0, vLen=0.0, length=0.0))
        self.curSel = code
        self.fillTable()
        self.rebuild(fit=False)
        self.win.select_dwg(CV, code)
        self.fillDetail(code)
        r = len(self.tableRows()) - 1
        self.tblSetSel(r)
        self.cellBegin(r, 0)
    def onDelRow(self):
        r = self.tblSel()
        if r < 0 or r >= len(self.tableRows()):
            self.win.set_text(L_EDIT_COUNT, '先在表格里点一行')
            return
        self.gridDeleteRow(r)

    def onApplyAll(self):
        """HTML edApply：收回表格里每一行的值，校验后重绘。"""
        nodes, err = self.collectGrid()
        if err:
            self.win.set_text(L_EDIT_COUNT, err)
            self.win.set_text(L_STATUS, err)
            return
        self.nodes = nodes
        self.rowEditClose()
        self.rebuild(fit=True)
        self.fillTable()
        self.win.set_text(L_EDIT_COUNT, '已按当前数据重绘')


def readText(path):
    with open(path, 'rb') as f:
        raw = f.read()
    for enc in ('utf-8-sig', 'utf-16', 'gbk'):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode('utf-8', 'replace')


def writeText(path, text, bom=False):
    data = text.encode('utf-8')
    if bom:
        data = b'\xef\xbb\xbf' + data
    with open(path, 'wb') as f:
        f.write(data)


def main():
    app = App()
    app.win.show()
    app.show_editor()      # HTML: 右侧干线表默认打开
    try:
        while True:
            M.pump(30)
            app.checkHotkeys()
            time.sleep(0.004)
    except KeyboardInterrupt:
        pass
    finally:
        app.win.close()


if __name__ == '__main__':
    main()
