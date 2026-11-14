# 生成 Office 风格 Ribbon 头部（选项卡条 + 分组大按钮 + 组名），纯 XML
import io, os

PATH = r'G:\PyArx\UIDesign\designer_layout.xml'

GROUPS = [
    ('文件', [
        ('btnNew', 'res/new.png', '新建'),
        ('btnOpen', 'res/open.png', '打开'),
        ('btnSave', 'res/save.png', '保存'),
    ]),
    ('剪贴板', [
        ('btnPaste', 'res/paste.png', '粘贴'),
        ('btnCut', 'res/cut.png', '剪切'),
        ('btnCopy', 'res/copy.png', '复制'),
        ('btnDelete', 'res/del.png', '删除'),
    ]),
    ('编辑', [
        ('btnUndo', 'res/undo.png', '撤销'),
        ('btnRedo', 'res/redo.png', '重做'),
    ]),
    ('对齐', [
        ('btnAlignLeft', 'res/alignleft.png', '左'),
        ('btnAlignHCenter', 'res/alignhcenter.png', '居中'),
        ('btnAlignRight', 'res/alignright.png', '右'),
        ('btnAlignTop', 'res/aligntop.png', '上'),
        ('btnAlignVCenter', 'res/alignvcenter.png', '居中'),
        ('btnAlignBottom', 'res/alignbottom.png', '下'),
    ]),
    ('分布', [
        ('btnDistH', 'res/disth.png', '横分布'),
        ('btnDistV', 'res/distv.png', '纵分布'),
    ]),
    ('视图', [
        ('btnGrid', 'res/grid.png', '网格'),
        ('btnRuler', 'res/ruler.png', '标尺'),
    ]),
    ('缩放', [
        ('btnZoomOut', 'res/zoomout.png', '缩小'),
        ('btnZoomIn', 'res/zoomin.png', '放大'),
    ]),
]

GROUP_IDS = {'文件': 'File', '剪贴板': 'Clip', '编辑': 'Edit', '对齐': 'Align',
             '分布': 'Dist', '视图': 'View', '缩放': 'Zoom'}

BTN_W, BTN_H, BTN_Y = 58, 60, 4
PAD_X, DIV_W = 12, 13

TABS = [('tabFile', '文件'), ('tabEdit', '编辑'), ('tabInsert', '插入'),
        ('tabLayout', '布局'), ('tabView', '视图'), ('tabHelp', '帮助')]


def build():
    lines = []
    lines.append('    <!-- ==================== 选项卡条（Ribbon：文件 / 开始 / 插入 …） ==================== -->')
    lines.append('    <ToolBar Grid.Row="1" Background="#232830" Foreground="#C8D2DF" FontSize="10"')
    lines.append('             BorderColor="#39404B" Spacing="0" Padding="10,0,10,0">')
    for name, text in TABS:
        sel = ' Selected="1"' if name == 'tabEdit' else ''
        lines.append('      <XButton Name="%s" Role="Tab" Text="%s" Width="66" Height="32"%s/>' % (name, text, sel))
    # 右端动作放在选项卡条上：Ribbon 区留给分组，避免大按钮被压住
    lines.append('      <XButton Name="btnCheck" Text="校验" Role="Ghost" Dock="Right" Width="68" Height="26" Margin="0,3,10,0" FontSize="10"/>')
    lines.append('      <XButton Name="btnPreview" Text="预览" Role="Primary" Dock="Right" Width="80" Height="26" Margin="0,3,16,0" FontSize="10"/>')
    lines.append('    </ToolBar>')
    lines.append('')
    lines.append('    <!-- ==================== Ribbon 功能区（分组 + 大图标按钮 + 组名） ====================')
    lines.append('         结构全部写在 XML：组的位置/宽度由 Left/Width 决定，组名是 Role="GroupTitle" 的 XLabel，')
    lines.append('         按钮图标与标题来自 Icon/Text。要增删组只改这里，C++ 不需要动。 -->')
    lines.append('    <ToolBar Grid.Row="2" Background="#20242C" Foreground="#E4E9F0" BorderColor="#39404B" Height="92">')

    x = PAD_X
    group_edges = []
    for title, items in GROUPS:
        gx = x
        for i, (name, icon, text) in enumerate(items):
            lines.append('      <XButton Name="%s" Icon="%s" Text="%s" Left="%d" Top="%d" Width="%d" Height="%d" FontSize="9"/>'
                         % (name, icon, text, gx + i * BTN_W, BTN_Y, BTN_W, BTN_H))
        gw = len(items) * BTN_W
        lines.append('      <XLabel Name="lblGroup%s" Role="GroupTitle" Text="%s" Left="%d" Top="68" Width="%d" Height="16" FontSize="9"/>'
                     % (GROUP_IDS.get(title, 'G'), title, gx, gw))
        x = gx + gw
        group_edges.append(x)
        lines.append('      <XDivider Left="%d" Top="8" Width="%d" Height="64" Thickness="1" Orientation="Vertical" Background="#39404B"/>'
                     % (x + DIV_W // 2 - 1, 1))
        x += DIV_W
    lines.append('    </ToolBar>')
    return '\n'.join(lines)


raw = io.open(PATH, 'r', encoding='utf-8-sig').read()

# 1) 行定义：38 / 54 → 38 / 34 / 92
old_rows = '      <RowDefinition Height="54"/>     <!-- 工具栏（图标 + 标题，分组） -->'
new_rows = ('      <RowDefinition Height="34"/>     <!-- 选项卡条（Ribbon 的 文件/开始/插入…） -->\n'
            '      <RowDefinition Height="92"/>     <!-- Ribbon 功能区（分组 + 大图标按钮 + 组名） -->')
assert old_rows in raw, 'row def not found'
raw = raw.replace(old_rows, new_rows)

# 2) 替换整个旧 ToolBar 段
start_mark = '    <!-- ==================== 工具栏（分组 + 图标标题 + 右侧动作） ==================== -->'
assert start_mark in raw, 'toolbar marker not found'
start = raw.index(start_mark)
end_mark = '    </ToolBar>'
end = raw.index(end_mark, start) + len(end_mark)
raw = raw[:start] + build() + raw[end:]

# 3) 新增一行后，rootGrid 外层子节点整体下移一行（按字符串定位，内层子网格不动）
SHIFT = [
    ('    <XGrid Grid.Row="2" Background="#15181D">', '    <XGrid Grid.Row="3" Background="#15181D">'),
    ('    <XSplitView Grid.Row="3"/>\n', '    <XSplitView Grid.Row="4"/>\n'),
    ('    <XGrid Name="codeDock" Grid.Row="4"', '    <XGrid Name="codeDock" Grid.Row="5"'),
    ('    <StatusBar Grid.Row="5"', '    <StatusBar Grid.Row="6"'),
]
for old, new in SHIFT:
    if old in raw:
        raw = raw.replace(old, new, 1)
        print('shifted:', old.strip()[:50])
    else:
        print('WARN not found:', old.strip()[:50])

io.open(PATH, 'w', encoding='utf-8-sig', newline='').write(raw)
print('ribbon xml written')
