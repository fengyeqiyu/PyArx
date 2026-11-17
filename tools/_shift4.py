import io

PATH = r'G:\PyArx\UIDesign\designer_layout.xml'
lines = io.open(PATH, encoding='utf-8-sig').read().split('\n')

# rootGrid 的外层子节点因新增选项卡行整体下移一行（行号基于本脚本生成后的稳定文件）
targets = {168: (2, 3), 356: (3, 4), 359: (4, 5), 371: (5, 6)}
for ln_no, (old, new) in targets.items():
    i = ln_no - 1
    lines[i] = lines[i].replace('Grid.Row="%d"' % old, 'Grid.Row="%d"' % new, 1)

name_map = {
    'lblGroup剪贴板': 'lblGroupClip', 'lblGroup编辑': 'lblGroupEdit',
    'lblGroup对齐': 'lblGroupAlign', 'lblGroup分布': 'lblGroupDist',
    'lblGroup视图': 'lblGroupView', 'lblGroup缩放': 'lblGroupZoom',
    'lblGroup文件': 'lblGroupFile',
}
text = '\n'.join(lines)
for old, new in name_map.items():
    text = text.replace(old, new)

io.open(PATH, 'w', encoding='utf-8-sig', newline='').write(text)
for ln_no in targets:
    print(ln_no, text.split('\n')[ln_no - 1].strip()[:70])
