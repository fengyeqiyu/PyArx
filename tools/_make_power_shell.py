# -*- coding: utf-8 -*-
"""生成配电规划图的 XML 外壳（examples/ui/power_plan_shell.xml）。

分工（新底层逻辑）：
  这份 XML 只声明"界面长什么样"：Ribbon 页签/分组/按钮、画布位置、右侧信息板、状态行。
  配电图的解析、排版、绘制、命中、缩放全在控件库（xmlctl::DwgCanvasView + xmlplan::drawPlan），
  设计器 / Viewer / 运行时读到的是同一份 XML、走的是同一份库代码，所以外观一致。
"""
import io, json

def esc(s):
    return (s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
             .replace('"', '&quot;').replace("'", '&apos;'))

# 一份真实感的配电场景：10kV 总进线 → 双电源柜 → 各末端负载
NODES = [
    ("P0",  "10kV 总进线",      "",    "YJV22-3×70",      0,    "10kV",  0),
    ("P1",  "1# 双电源柜",      "P0",  "YJV22-4×95",      45,   "380V",  22),
    ("P2",  "2# 配电柜",        "P0",  "YJV22-4×70",      36,   "380V",  18),
    ("T11", "中试车间照明",     "P1",  "YJV22-4×35",      11,   "380V",  12),
    ("T12", "车间动力 1#",      "P1",  "YJV22-4×50",      22,   "380V",  26),
    ("T13", "涂布机主控",       "P1",  "YJV22-4×25",      8.5,  "380V",  10),
    ("T21", "空压站",           "P2",  "YJV22-4×35",      15,   "380V",  14),
    ("T22", "空调机房",         "P2",  "YJV22-4×50",      30,   "380V",  32),
    ("T23", "办公区照明",       "P2",  "YJV22-4×16",      6,    "220V",  6),
    ("T24", "UPS 机房",         "P2",  "YJV22-4×25",      9,    "380V",  8),
    ("L111","车间插座回路",     "T11", "YJV22-4×16",      3.5,  "220V",  3),
    ("L112","应急照明",         "T11", "YJV22-4×10",      1.5,  "220V",  2),
]

DATA = {
    "dir": "h",
    "nodes": [
        {"code": c, "name": nm, "upstream": up, "cable": cb, "loadKw": kw,
         "voltage": vt, "length": ln}
        for (c, nm, up, cb, kw, vt, ln) in NODES
    ]
}
data_attr = esc(json.dumps(DATA, ensure_ascii=False, separators=(',', ':')).replace('\r', ''))

TABS = [
    ("编辑", [
        ("导入", [("btnImport", "导入", "open.png"), ("btnExport", "导出", "save.png"),
                  ("btnExportPdf", "导出 PDF", "save.png")]),
        ("打印", [("btnPrint", "打印", "ok.png"), ("btnPreview", "预览", "fullscreen.png")]),
        ("编辑", [("btnUndo", "撤销", "undo.png"), ("btnRedo", "重做", "redo.png")]),
    ]),
    ("视图", [
        ("缩放", [("btnZoomFit", "适应窗口", "zoomfit.png"), ("btnZoomIn", "放大", "zoomin.png"),
                  ("btnZoomOut", "缩小", "zoomout.png"), ("btnZoomReset", "100%", "zoomreset.png")]),
        ("显示", [("btnCable", "电缆标注", "ok.png"), ("btnGrid", "网格", "grid.png"),
                  ("btnRuler", "标尺", "ruler.png"), ("btnMini", "小地图", "fullscreen.png")]),
    ]),
    ("数据", [
        ("设备", [("btnDevice", "设备表", "grid.png"), ("btnLoop", "回路表", "paste.png"),
                  ("btnRelayout", "重新排版", "alignleft.png")]),
        ("校验", [("btnCross", "交叉校验", "ok.png"), ("btnStat", "统计", "about.png")]),
    ]),
    ("帮助", [
        ("支持", [("btnDoc", "使用说明", "open.png"), ("btnAbout", "关于", "about.png")]),
    ]),
]


def ribbon():
    out = ['    <XRibbon Name="ppRibbon" Height="126" SelectedTab="0">']
    for hdr, groups in TABS:
        out.append('      <XRibbonTab Header="%s">' % hdr)
        for gname, btns in groups:
            out.append('        <XRibbonGroup Header="%s">' % gname)
            for name, text, icon in btns:
                out.append('          <XRibbonButton Name="%s" Icon="res/%s" Text="%s"/>'
                           % (name, icon, text))
            out.append('        </XRibbonGroup>')
        out.append('      </XRibbonTab>')
    out.append('    </XRibbon>')
    return '\n'.join(out)


xml = '''<?xml version="1.0" encoding="utf-8"?>
<!-- 配电规划图 —— 完整的 XML 外壳（新底层逻辑）：
     外观 = 这份 XML；能力 = 控件库（Ribbon / XDwgCanvas / 面板 / 状态行）；
     业务 = 运行时绑定的命令。设计器里打开、Viewer 里预览、Python 里运行是同一份文件。
     画布数据 Data 只用 nodes（不动坐标），排版/命中/缩放全部由控件库算。 -->
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xml"
        Title="配电规划图" Width="1320" Height="820" Background="#FFF3F5F8">

  <XStackPanel>
    %s

    <!-- 主区：左画布 / 右信息板 -->
    <XStackPanel Orientation="Horizontal" Height="660">
      <XDwgCanvas Name="Plan" Width="978" Height="660" ShowCable="1" Data="%s"/>

      <XStackPanel Width="336" Height="660">
        <XStackPanel Orientation="Horizontal" Height="34">
          <XLabel Content="选中设备" Width="120" Height="34"/>
          <XLabel Name="lblSelCode" Content="（未选中）" Height="34"/>
        </XStackPanel>
        <XDivider Height="1"/>
        <XLabel Name="lblSelName" Content="名称" Height="26"/>
        <XLabel Name="lblSelVolt" Content="电压等级" Height="26"/>
        <XLabel Name="lblSelLoad" Content="负荷" Height="26"/>
        <XLabel Name="lblSelCable" Content="电缆" Height="26"/>
        <XLabel Name="lblSelLen" Content="长度" Height="26"/>
        <XDivider Height="1"/>
        <XLabel Content="操作提示" Height="30"/>
        <XLabel Content="滚轮：缩放（以光标为锚点）" Height="24"/>
        <XLabel Content="中键拖动：平移" Height="24"/>
        <XLabel Content="左键单击：选中设备/回路" Height="24"/>
        <XLabel Content="Ribbon「视图 → 适应窗口」复位" Height="24"/>
      </XStackPanel>
    </XStackPanel>

    <!-- 底部状态行 -->
    <XStackPanel Orientation="Horizontal" Height="26">
      <XLabel Name="lblState" Content="就绪 · 12 台设备 · 11 条回路" Height="26"/>
    </XStackPanel>
  </XStackPanel>
</XWindow>
''' % (ribbon(), data_attr)

p = r'G:\PyArx\examples\ui\power_plan_shell.xml'
io.open(p, 'w', encoding='utf-8', newline='\r\n').write(xml)
print('written', p, len(xml))
