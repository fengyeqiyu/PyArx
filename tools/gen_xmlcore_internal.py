import re, pathlib

ROOT = pathlib.Path(r'G:\PyArx\MfcXmlCore')
HEADER = ROOT / 'XmlCoreInternal.h'
TU = sorted(ROOT.glob('XmlCore.*.cpp'))

SYMS = ['ApplyRowFields', 'ApplyStyleFlexProps', 'AttachPanel', 'AttrIsAuto', 'BaselineFromTop', 'ClearEffective',
        'DefaultHeight', 'defaultSizeFor', 'ElHidden', 'ExpandDelegate', 'FindByName', 'g_layoutDbg', 'HasLocal',
        'IntAttr', 'KindOf', 'layoutDefaults', 'LayoutNode', 'LayoutNodeAt', 'LocalName', 'MarkDelegateRow',
        'NormPath', 'ParseElement', 'ParsePadding', 'ParseThickness', 'PathDirOf', 'SetEffective', 'SyncText',
        'XmlEscape']
HTML_SYMS = {'ApplyStyleFlexProps'}

TYPES = '''    // 流式布局上下文（原 02_util 末尾）
    struct XmlFlow
    {
        int margin = 0;      // 不再有暗默认边距：位置一律来自 XML 的 Left/Top/Margin
        int spacing = 0;     // 子元素间距，来自容器 XML 的 Spacing（缺省 0）
        int cursorY = 0;     // 相对 offsetY
        int cursorX = 0;     // 横向排列游标
        int width = 240;     // 容器内容宽度
        int offsetX = 0;
        int offsetY = 0;
        bool horizontal = false;
        int height = 0;      // 容器可用高度
        int padding = 0;     // 左内边距（XML Padding）
        int paddingY = 0;    // 上内边距（XML Padding）
        std::vector<int> gridRows;
        std::vector<int> gridCols;
    };

    using TemplateTable = std::vector<std::pair<std::wstring, std::vector<XmlElement>>>;

    struct LayoutDefaults
    {
        std::vector<std::pair<std::wstring, std::pair<int, int>>> sizes; // tag -> (w,h)
        int starMin = 30;      // 星号轨道最小宽度
        int groupLeft = 12;    // GroupBox 边框物理内区左偏移
        int groupTop = 24;     // GroupBox 边框物理内区上偏移
    };
'''

def masked(text):
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        if text[i] == '/' and i + 1 < n and text[i+1] == '/':
            while i < n and text[i] != chr(10):
                out[i] = ' '; i += 1
        elif text[i] == '/' and i + 1 < n and text[i+1] == '*':
            while i < n and not (text[i] == '*' and i + 1 < n and text[i+1] == '/'):
                if text[i] != chr(10): out[i] = ' '
                i += 1
            for k in range(i, min(i+2, n)): out[k] = ' '
            i += 2
        elif text[i] == '"' :
            out[i] = ' '; i += 1
            while i < n and text[i] != '"':
                if text[i] == chr(92): i += 1
                if i < n and text[i] != chr(10): out[i] = ' '
                i += 1
            if i < n: out[i] = ' '; i += 1
        else:
            i += 1
    return ''.join(out)

NS_OPEN = re.compile(r'namespace\s+xmlcore_impl\s*\r?\n?\s*\{')


def inside_impl(text, offset):
    mtext = masked(text)
    for m in NS_OPEN.finditer(mtext):
        brace = mtext.index('{', m.start())
        brace = text.index('{', m.start())
        d, j = 0, brace
        while j < len(mtext):
            if mtext[j] == '{':
                d += 1
            elif mtext[j] == '}':
                d -= 1
                if d == 0:
                    break
            j += 1
        if brace <= offset <= j:
            return True
    return False


def defpat(n):
    return re.compile(r'(?m)^[ \t]*(?:(inline|static)\s+)?[A-Za-z_][\w:<>,\*&\s]*[ \t\*&]+' + re.escape(n) + r'\s*\(')


def strip_defaults(sig):
    return re.sub(r'\s*=\s*[^,()]*?(?=[,)])', '', sig)


def locate(name):
    for f in TU:
        text = f.read_text(encoding='utf-8-sig')
        for m in defpat(name).finditer(text):
            i = m.end() - 1
            d = 0
            while i < len(text):
                if text[i] == '(':
                    d += 1
                elif text[i] == ')':
                    d -= 1
                    if d == 0:
                        break
                i += 1
            j = i + 1
            while j < len(text) and text[j] in ' \t\r\n':
                j += 1
            if j >= len(text) or text[j] not in '{:':
                continue
            sig = re.sub(r'^(static|inline)\s+', '', text[m.start():i+1].strip())
            sig = strip_defaults(sig)
            qual = text[i+1:j].strip()
            ns = 'htmlsugar' if name in HTML_SYMS else ('xmlcore_impl' if inside_impl(text, m.start()) else '')
            return ns, sig + (' ' + qual if qual else '') + ';'
        vm = re.search(r'(?m)^[ \t]*(?:const|extern)?\s*[A-Za-z_][\w:<>,\*& ]*\s' + re.escape(name) + r'\s*[=;]', text)
        if vm:
            head = text[vm.start():text.index(';', vm.start())].split('=')[0].strip()
            head = re.sub(r'^extern\s+', '', head)
            ns = 'xmlcore_impl' if inside_impl(text, vm.start()) else ''
            return ns, 'extern ' + head + ';'
    return None


found, missing = [], []
for s in SYMS:
    r = locate(s)
    if r:
        found.append((s, r[0], r[1]))
    else:
        missing.append(s)
print('found', len(found), 'missing', missing)

impl = [d for s, ns, d in found if ns == 'xmlcore_impl']
glob = [d for s, ns, d in found if ns == '']
html = [d for s, ns, d in found if ns == 'htmlsugar']

hdr = ['// XmlCoreInternal.h —— 跨 TU 内部声明（拆 TU 后原先匿名 namespace 的共享件）。',
       '// 类型定义在前（多 TU 共用同一份布局），函数只声明、定义留在各自 .cpp。',
       '#pragma once', '#include "XmlCoreCommon.h"', '',
       'namespace xmlcore_impl', '{', TYPES, '']
hdr += ['    ' + d for d in impl] + ['}', '', 'using namespace xmlcore_impl;   // 各 TU 保持原代码的裸名写法', '']
if glob:
    hdr += ['// 定义在全局作用域的共享辅助（Runtime / Style 等 TU）'] + glob + ['']
if html:
    hdr += ['namespace htmlsugar', '{'] + ['    ' + d for d in html] + ['}']
hdr += []
HEADER.write_text('\n'.join(hdr) + '\n', encoding='utf-8-sig')
print('written; impl', len(impl), 'global', len(glob), 'html', len(html))
