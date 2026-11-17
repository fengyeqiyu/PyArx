import io, re

PATH = r'G:\PyArx\UIDesign\designer_layout.xml'
lines = io.open(PATH, 'r', encoding='utf-8-sig').read().split('\n')

NAME_MAP = {
    'lblGroup剪贴板': 'lblGroupClip', 'lblGroup编辑': 'lblGroupEdit',
    'lblGroup对齐': 'lblGroupAlign', 'lblGroup分布': 'lblGroupDist',
    'lblGroup视图': 'lblGroupView', 'lblGroup缩放': 'lblGroupZoom',
    'lblGroup文件': 'lblGroupFile',
}

out = []
shifted = []
for ln in lines:
    for old, new in NAME_MAP.items():
        ln = ln.replace(old, new)

    # 只有 rootGrid 的直接子节点（缩进正好 6 空格）才整体下移一行；
    # 子网格内部的 Grid.Row 保持原值
    indent = len(ln) - len(ln.lstrip(' '))
    if indent == 6:
        m = re.search(r'Grid\.Row="(\d+)"', ln)
        if m and int(m.group(1)) >= 2:
            n = int(m.group(1))
            ln = ln.replace(m.group(0), 'Grid.Row="%d"' % (n + 1))
            shifted.append((n, ln.strip()[:60]))
    out.append(ln)

io.open(PATH, 'w', encoding='utf-8-sig', newline='').write('\n'.join(out))
print('shifted outer rows:', len(shifted))
for s in shifted:
    print('  ', s[1])
