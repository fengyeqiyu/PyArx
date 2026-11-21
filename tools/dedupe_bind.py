import re, pathlib

src = pathlib.Path(r'G:\PyArx\log\PyMfcBind.before_dedupe.cpp')
text = src.read_text(encoding='utf-8')

DECL = r'py::class_<PyMfcWindow, PyMfcHandle<PyMfcWindow>>(m, "Window")'
PDECL = r'py::class_<PyMfcPanel, PyMfcHandle<PyMfcPanel>>(m, "Pane")'
i_win = text.index(DECL) + len(DECL)
i_pan = text.index(PDECL)
i_pump = text.index('    m.def("pump"')
panm = re.search(r'\n    m\.def\(', text[i_pan + len(PDECL):])
i_pan_end = i_pan + len(PDECL) + panm.start()

win_block = text[i_win:i_pan]
pan_block = text[i_pan + len(PDECL):i_pan_end]

ENTRY = re.compile(r'\.def(?:_property(?:_readonly)?|_static)?\s*\(')

def entries(block):
    pos = [m.start() for m in ENTRY.finditer(block)]
    out = []
    for i, p in enumerate(pos):
        e = pos[i+1] if i+1 < len(pos) else len(block)
        chunk = block[p:e].rstrip()
        m = re.match(r'\.def(?:_property(?:_readonly)?|_static)?\s*\(\s*"([^"]+)"', chunk)
        name = m.group(1) if m else '?' + str(i)
        out.append((name, chunk))
    return out

win_e = entries(win_block)
pan_e = entries(pan_block)
pan_map = dict(pan_e)

shared = []
win_only = []
for n, c in win_e:
    pc = pan_map.get(n)
    if pc and '&PyMfcWindow::' in c and '&PyMfcPanel::' in pc and '[' not in c and n not in ('move',):
        shared.append((n, c))
    else:
        win_only.append((n, c))
pan_only = [(n, c) for n, c in pan_e if n not in {x for x, _ in shared}]

print('win', len(win_e), 'pan', len(pan_e), 'shared', len(shared), 'win_only', len(win_only), 'pan_only', len(pan_only))
print('win_only:', [n for n, _ in win_only][:30])
print('pan_only:', [n for n, _ in pan_only][:30])

def clean(chunk):
    lines = [ln for ln in chunk.splitlines() if not ln.strip().startswith('//')]
    s = '\n'.join(lines).rstrip()
    while s.endswith(';'):
        s = s[:-1].rstrip()
    return s

def tpl_body(c):
    t = clean(c).replace('&PyMfcWindow::', '&C::')
    t = re.sub(r'^\s+', '', t)
    return t

tpl = ['template <class C, class Cls>', 'static void RegisterSurfaceApi(Cls& cls)', '{']
for _, c in shared:
    tpl.append('    cls' + tpl_body(c) + ';')
tpl.append('}')
tpl_text = '\n'.join(tpl) + '\n'

def emit(entries_list, recv):
    out = []
    for _, c in entries_list:
        out.append('    ' + recv + re.sub(r'^\s+', '', clean(c)) + ';')
    return out

pre = text[:text.index(DECL)]
pre = pre[:pre.rindex('\nPYBIND11_MODULE')] if '\nPYBIND11_MODULE' in pre else pre
modhead = text[text.rindex('\nPYBIND11_MODULE'):text.index(DECL)]
# keep everything from module opening through class decl start
mod = modhead.rstrip() + '\n'
mod += '    auto clsWin = ' + DECL + ';\n'
mod += '    RegisterSurfaceApi<PyMfcWindow>(clsWin);\n'
mod += '\n'.join(emit(win_only, 'clsWin')) + '\n'
mod += '    auto clsPane = ' + PDECL + ';\n'
mod += '    RegisterSurfaceApi<PyMfcPanel>(clsPane);\n'
mod += '\n'.join(emit(pan_only, 'clsPane')) + '\n'
i_mdef = text.find('\n    m.def(', text.index(PDECL))
mod_defs_block = text[i_mdef:i_pump].lstrip('\n')
mod += mod_defs_block.rstrip('\n') + '\n'
rest = text[i_pump:]

final = pre.rstrip('\n') + '\n\n' + tpl_text + '\n' + mod + '\n' + rest
pathlib.Path(r'G:\PyArx\PyMfcXml\PyMfcBind.gen.cpp').write_text(final, encoding='utf-8')
print('lines', final.count('\n'))
