import io, re, os

D = 'G:/PyArx/PyMfcXaml'


def scan(lines_):
    out = []
    state = None
    for line in lines_:
        dlt = 0
        opened = False
        i = 0
        while i < len(line):
            ch = line[i]
            nx = line[i + 1] if i + 1 < len(line) else ''
            if state == 'lc':
                break
            if state == 'bc':
                if ch == '*' and nx == '/':
                    state = None; i += 2; continue
                i += 1; continue
            if state == 's':
                if ch == '\\':
                    i += 2; continue
                if ch == '"':
                    state = None
                i += 1; continue
            if state == 'c':
                if ch == '\\':
                    i += 2; continue
                if ch == "'":
                    state = None
                i += 1; continue
            if ch == '/' and nx == '/':
                state = 'lc'; i += 2; continue
            if ch == '/' and nx == '*':
                state = 'bc'; i += 2; continue
            if ch == '"':
                state = 's'; i += 1; continue
            if ch == "'":
                state = 'c'; i += 1; continue
            if ch == '{':
                dlt += 1; opened = True
            elif ch == '}':
                dlt -= 1
            i += 1
        if state == 'lc':
            state = None
        out.append((dlt, opened))
    return out


def strip_comment(s):
    i = s.find('//')
    return s if i < 0 else s[:i]


def endsemi(line):
    return strip_comment(line).strip().endswith(';')


def find_params(sig):
    for i, ch in enumerate(sig):
        if ch != '(':
            continue
        dep = 0
        j = i
        while j < len(sig):
            if sig[j] == '(':
                dep += 1
            elif sig[j] == ')':
                dep -= 1
                if dep == 0:
                    break
            j += 1
        t = (sig[j + 1:] if j < len(sig) else '').strip()
        if re.fullmatch(r'(const|override|final|noexcept|volatile|&&|\s|\(\)|->.*|=.*|:.*|&)*', t):
            return i, j
    return -1, -1


def drop_defaults(params):
    out = []
    dep = 0
    skip = False
    for ch in params:
        if skip:
            if dep == 0 and ch == ',':
                skip = False
                out.append(ch)
            continue
        if ch == '=':
            skip = True
            continue
        if ch in '(<[':
            dep += 1
        elif ch in ')>]':
            dep -= 1
        out.append(ch)
    return ''.join(out).rstrip().rstrip(',').rstrip()


def split_class(body_lines, cls, nested=()):
    """返回 (头文件类体文本, 成员定义文本列表, 失败段数)"""
    sc = scan(body_lines)
    hdr, defs, fails = [], [], 0
    i = 0
    while i < len(body_lines):
        line = body_lines[i]
        s = line.strip()
        if not s:
            hdr.append(line); i += 1; continue
        if s in ('public:', 'private:', 'protected:') or s.startswith(('//', '/*', '*')):
            hdr.append(line); i += 1; continue
        # 嵌套类型/宏/using/template
        if re.match(r'(struct|class|enum|union|using|typedef|friend|template)\b', s) or s.startswith('DECLARE_'):
            start = i
            dep = 0; opened = False
            while i < len(body_lines):
                dep += sc[i][0]
                opened = opened or sc[i][1]
                i += 1
                if (not opened and endsemi(body_lines[i - 1])) or (opened and dep == 0):
                    break
            hdr.extend(body_lines[start:i])
            continue
        # 数据成员
        if endsemi(line) and not sc[i][1]:
            hdr.append(line); i += 1; continue
        # 函数定义
        start = i
        dep = 0; opened = False
        while i < len(body_lines):
            dep += sc[i][0]
            opened = opened or sc[i][1]
            i += 1
            if opened and dep == 0:
                break
        chunk = body_lines[start:i]
        k = None
        for j in range(len(chunk)):
            if sc[start + j][1]:
                k = j; break
        if k is None:
            hdr.extend(chunk); fails += 1; continue
        brace_col = chunk[k].find('{')
        sig_lines = chunk[:k] + [chunk[k][:brace_col]]
        while sig_lines and sig_lines[0].strip().startswith(('//', '/*', '*')):
            hdr.append(sig_lines[0]); sig_lines = sig_lines[1:]
        sig = '\n'.join(sig_lines).strip()
        body_txt = chunk[k][brace_col:] + '\n' + '\n'.join(chunk[k + 1:])
        if not re.search(r'\w\s*\(', sig):
            hdr.extend(chunk); fails += 1; continue
        p_open, p_close = find_params(sig)
        if p_open < 0:
            hdr.extend(chunk); fails += 1; continue
        head_part = sig[:p_open]
        m = re.search(r'([~]?\w+)\s*$', head_part)
        if not m:
            hdr.extend(chunk); fails += 1; continue
        name = m.group(1)
        ret = head_part[:m.start()].strip()
        params = sig[p_open + 1:p_close]
        suffix = sig[p_close + 1:].strip()
        if re.search(r'=\s*(default|delete|0)', suffix):
            hdr.extend(chunk); continue
        hdr.append(sig.rstrip() + ';')
        ret_cpp = ret
        _kw = {'static', 'virtual', 'inline', 'explicit', 'constexpr', 'mutable', 'friend'}
        _parts = ret_cpp.split(None, 1)
        while _parts and _parts[0] in _kw:
            ret_cpp = _parts[1].strip() if len(_parts) > 1 else ''
            _parts = ret_cpp.split(None, 1)
        suffix_cpp = re.sub(r'\b(override|final)\b', '', suffix).strip()
        sig_cpp = (ret_cpp + (' ' if ret_cpp else '') + cls + '::' + name
                   + '(' + drop_defaults(params) + ')'
                   + ((' ' + suffix_cpp) if suffix_cpp else ''))
        for n in nested:
            sig_cpp = re.sub(r'(?<![\w:])' + n + r'(?![\w])', cls + '::' + n, sig_cpp)
        defs.append(sig_cpp + '\n' + body_txt)
    return '\n'.join(hdr), defs, fails
