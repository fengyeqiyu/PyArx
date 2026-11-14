# 把设计器头部从"两个 ToolBar 手工排坐标"换成控件库的 <XRibbon>
import io

PATH = r'G:\PyArx\UIDesign\designer_layout.xml'
raw = io.open(PATH, encoding='utf-8-sig').read()

# 1) 行定义：选项卡条 + 功能区合成一行（Ribbon 控件自己分上/下两段）
old_rows = ('      <RowDefinition Height="34"/>     <!-- 选项卡条（Ribbon 的 文件/开始/插入…） -->\n'
            '      <RowDefinition Height="92"/>     <!-- Ribbon 功能区（分组 + 大图标按钮 + 组名） -->')
new_rows = '      <RowDefinition Height="126"/>    <!-- Ribbon（选项卡条 34 + 功能区 92，由 XRibbon 控件自绘） -->'
assert old_rows in raw, 'row defs not found'
raw = raw.replace(old_rows, new_rows)

GROUPS = {
    '文件': [
        ('btnNew', 'res/new.png', '新建'),
        ('btnOpen', 'res/open.png', '打开'),
        ('btnSave', 'res/save.png', '保存'),
    ],
    '剪贴板': [
        ('btnPaste', 'res/paste.png', '粘贴'),
        ('btnCut', 'res/cut.png', '剪切'),
        ('btnCopy', 'res/copy.png', '复制'),
        ('btnDelete', 'res/del.png', '删除'),
    ],
    '编辑': [
        ('btnUndo', 'res/undo.png', '撤销'),
        ('btnRedo', 'res/redo.png', '重做'),
    ],
    '对齐': [
        ('btnAlignLeft', 'res/alignleft.png', '左'),
        ('btnAlignHCenter', 'res/alignhcenter.png', '居中'),
        ('btnAlignRight', 'res/alignright.png', '右'),
        ('btnAlignTop', 'res/aligntop.png', '上'),
        ('btnAlignVCenter', 'res/alignvcenter.png', '居中'),
        ('btnAlignBottom', 'res/alignbottom.png', '下'),
    ],
    '分布': [
        ('btnDistH', 'res/disth.png', '横分布'),
        ('btnDistV', 'res/distv.png', '纵分布'),
    ],
    '视图': [
        ('btnGrid', 'res/grid.png', '网格'),
        ('btnRuler', 'res/ruler.png', '标尺'),
    ],
    '缩放': [
        ('btnZoomOut', 'res/zoomout.png', '缩小'),
        ('btnZoomIn', 'res/zoomin.png', '放大'),
    ],
}

block = []
block.append('    <!-- ==================== Ribbon（控件库 XRibbon：选项卡 / 分组 / 大按钮 / 组名） ====================')
block.append('         结构见 MfcXmlCore\\XmlRibbon.h：选项卡 → 分组 → 按钮。要增删按钮只改这份 XML，')
block.append('         绘制、命中、切页都由控件库完成，设计器只负责把命中转成动作名。 -->')
block.append('    <XRibbon Name="mainRibbon" Grid.Row="1" SelectedTab="1">')

def emit_tab(header, group_names):
    block.append('')
    block.append('      <XRibbonTab Header="%s">' % header)
    for gname in group_names:
        block.append('        <XRibbonGroup Header="%s">' % gname)
        for name, icon, text in GROUPS[gname]:
            block.append('          <XRibbonButton Name="%s" Icon="%s" Text="%s"/>' % (name, icon, text))
        block.append('        </XRibbonGroup>')
    block.append('      </XRibbonTab>')

emit_tab('文件', ['文件'])
emit_tab('编辑', ['剪贴板', '编辑', '对齐', '分布', '视图', '缩放'])
emit_tab('插入', ['剪贴板'])
emit_tab('布局', ['对齐'])
emit_tab('视图', ['视图', '缩放'])
emit_tab('帮助', ['文件'])
block.append('')
block.append('    </XRibbon>')

# 2) 替换两段 ToolBar（从"选项卡条"注释起，到第二个 </ToolBar> 结束）
start_mark = '    <!-- ==================== 选项卡条（Ribbon：文件 / 开始 / 插入 …） ==================== -->'
assert start_mark in raw, 'tab toolbar marker not found'
start = raw.index(start_mark)
end_mark = '    </ToolBar>'
first = raw.index(end_mark, start) + len(end_mark)          # 选项卡条结束
second = raw.index(end_mark, first) + len(end_mark)         # 功能区结束
raw = raw[:start] + '\n'.join(block) + raw[second:]

# 3) 行数减一（两行合成一行），外层行号整体上移一行
SHIFT = [
    ('    <XGrid Grid.Row="3" Background="#15181D">', '    <XGrid Grid.Row="2" Background="#15181D">'),
    ('    <XSplitView Grid.Row="4"/>\n', '    <XSplitView Grid.Row="3"/>\n'),
    ('    <XGrid Name="codeDock" Grid.Row="5"', '    <XGrid Name="codeDock" Grid.Row="4"'),
    ('    <StatusBar Grid.Row="6"', '    <StatusBar Grid.Row="5"'),
]
for old, new in SHIFT:
    assert old in raw, 'shift target not found: ' + old
    raw = raw.replace(old, new, 1)

io.open(PATH, 'w', encoding='utf-8-sig', newline='').write(raw)
print('XRibbon xml written')
