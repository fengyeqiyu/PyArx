import re, pathlib, sys

ROOT = pathlib.Path(r'G:\PyArx\MfcXmlCore')
HEADER = ROOT / 'XmlCoreInternal.h'
TU = sorted(ROOT.glob('XmlCore.*.cpp'))
names = sys.argv[1:]

NS_OPEN = re.compile(r'namespace\s+xmlcore_impl')
DEF = lambda n: re.compile(r'(?m)^[ \t]*(?:(inline|static)\s+)?[A-Za-z_][\w:<>,\*&\s]*[ \t\*&]+' + re.escape(n) + r'\s*\(')


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
            i += 2
        elif text[i] == '"':
            out[i] = ' '; i += 1
            while i < n and text[i] != '"':
                if text[i] == chr(92): i += 1
                if i < n and text[i] != chr(10): out[i] = ' '
                i += 1
            if i < n: out[i] = ' '; i += 1
        else:
            i += 1
    return ''.join(out)


def inside_impl(text, offset):
    mt = masked(text)
    for m in NS_OPEN.finditer(mt):
        brace = mt.index('{', m.start())
        d, j = 0, brace
        while j < len(mt):
            if mt[j] == '{':
                d += 1
            elif mt[j] == '}':
                d -= 1
                if d == 0:
                    break
            j += 1
        if brace <= offset <= j:
            return True
    return False


def locate(name):
    for f in TU:
        text = f.read_text(encoding='utf-8-sig')
        for m in DEF(name).finditer(text):
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
            sig = re.sub(r'\s*=\s*[^,()]*?(?=[,)])', '', sig)
            qual = text[i+1:j].strip()
            ns = 'impl' if inside_impl(text, m.start()) else 'global'
            return ns, sig + (' ' + qual if qual else '') + ';'
        sm = re.search(r'(?m)^[ \t]*struct\s+' + re.escape(name) + r'\b', text)
        if sm:
            i = text.index('{', sm.start())
            d, j = 0, i
            while j < len(text):
                if text[j] == '{':
                    d += 1
                elif text[j] == '}':
                    d -= 1
                    if d == 0:
                        break
                j += 1
            k = text.index(';', j)
            blk = text[sm.start():k+1]
            ns = 'impl' if inside_impl(text, sm.start()) else 'global'
            return ns, blk
    return None


add_impl, add_glob = [], []
for n in names:
    r = locate(n)
    if not r:
        print('UNRESOLVED', n); continue
    (add_impl if r[0] == 'impl' else add_glob).append(r[1])

t = HEADER.read_text(encoding='utf-8-sig')
if add_impl:
    t = t.rstrip('\n') + '\n\nnamespace xmlcore_impl\n{\n' + '\n'.join('    ' + d.replace('\n', '\n    ') for d in add_impl) + '\n}\n'
if add_glob:
    t = t.rstrip('\n') + '\n\n// 全局作用域共享辅助\n' + '\n'.join(add_glob) + '\n'
HEADER.write_text(t, encoding='utf-8-sig')
print('added impl', len(add_impl), 'global', len(add_glob))
