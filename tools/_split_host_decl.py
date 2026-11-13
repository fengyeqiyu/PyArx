import io, re, os

d = r'G:\PyArx\PyMfcXaml'
src = os.path.join(d, 'PyMfcHost.h')
lines = io.open(src, encoding='utf-8').read().split('\n')

# 预处理：源码里有一行把 "}" 和下一个方法签名粘在一起（}    std::wstring getCanvasCommands(...)）
# 先拆开，否则按行解析会把两段搅在一起。
fixed = []
for l in lines:
    m = re.match(r'(\s*\})\s{2,}([^\s/].*)$', l)
    if m:
        fixed.append(m.group(1))
        fixed.append(m.group(2))
    else:
        fixed.append(l)
lines = fixed

ci = next(i for i, l in enumerate(lines) if l.startswith('class PyMfcHost'))
oi = next(i for i in range(ci, len(lines)) if lines[i].strip() == '{')


def scan(lines_):
    """逐行扫描，返回 [(brace_delta, has_open_brace, signature_text_if_any)]，正确跳过注释/字符串。"""
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
                    state = None
                    i += 2
                    continue
                i += 1
                continue
            if state == 's':
                if ch == '\\':
                    i += 2
                    continue
                if ch == '"':
                    state = None
                i += 1
                continue
            if state == 'c':
                if ch == '\\':
                    i += 2
                    continue
                if ch == "'":
                    state = None
                i += 1
                continue
            if ch == '/' and nx == '/':
                state = 'lc'
                i += 2
                continue
            if ch == '/' and nx == '*':
                state = 'bc'
                i += 2
                continue
            if ch == '"':
                state = 's'
                i += 1
                continue
            if ch == "'":
                state = 'c'
                i += 1
                continue
            if ch == '{':
                dlt += 1
                opened = True
            elif ch == '}':
                dlt -= 1
            i += 1
        if state == 'lc':
            state = None
        out.append((dlt, opened))
    return out


# 类体结束：从 oi 开始累计
sc_all = scan(lines)
depth = 0
started = False
end = None
for i in range(oi, len(lines)):
    depth += sc_all[i][0]
    if sc_all[i][1]:
        started = True
    if started and depth == 0:
        end = i
        break

head = lines[:oi + 1]
body = lines[oi + 1:end]
tail = lines[end:]
sc = scan(body)

NESTED = ['Item', 'PlanCanvasState', 'AnimTrack']
ACCESS = ('public:', 'private:', 'protected:')

hdr_out = []
cpp_defs = []
n_method = 0
n_fail = 0


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
        tail_txt = sig[j + 1:] if j < len(sig) else ''
        t = tail_txt.strip()
        ok = re.fullmatch(r'(const|override|final|noexcept|volatile|&&|\s|\(\)|->.*|=.*|:.*)*', t)
        if ok:
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


def qual_nested(txt):
    for n in NESTED:
        txt = re.sub(r'(?<![\w:])' + n + r'(?![\w])', 'PyMfcHost::' + n, txt)
    return txt


i = 0
while i < len(body):
    line = body[i]
    s = line.strip()

    if not s:
        hdr_out.append(line)
        i += 1
        continue

    # 类体层级靠"整块消费"保证：走到这里的一定是直接成员行（缩进不统一，不能靠缩进判断）

    if s in ACCESS or s.startswith(('//', '/*', '*')):
        hdr_out.append(line)
        i += 1
        continue

    if re.match(r'(struct|class|enum|union|using|typedef|friend|template)\b', s) or s.startswith('DECLARE_'):
        start = i
        dep = 0
        opened = False
        while i < len(body):
            dep += sc[i][0]
            opened = opened or sc[i][1]
            i += 1
            if (not opened and endsemi(body[i - 1])) or (opened and dep == 0):
                break
        hdr_out.extend(body[start:i])
        continue

    if endsemi(line) and not sc[i][1]:
        hdr_out.append(line)
        i += 1
        continue

    start = i
    dep = 0
    opened = False
    while i < len(body):
        dep += sc[i][0]
        opened = opened or sc[i][1]
        i += 1
        if opened and dep == 0:
            break
    chunk = body[start:i]

    k = None
    for j in range(len(chunk)):
        if sc[start + j][1]:
            k = j
            break
    if k is None:
        hdr_out.extend(chunk)
        n_fail += 1
        continue

    brace_col = chunk[k].find('{')
    sig_lines = chunk[:k] + [chunk[k][:brace_col]]

    # 签名上方紧贴的注释属于声明区，留在头里；不参与解析
    while sig_lines and sig_lines[0].strip().startswith(('//', '/*', '*')):
        hdr_out.append(sig_lines[0])
        sig_lines = sig_lines[1:]

    sig = '\n'.join(sig_lines).strip()
    body_txt = chunk[k][brace_col:] + '\n' + '\n'.join(chunk[k + 1:])

    if not re.search(r'\w\s*\(', sig):
        hdr_out.extend(chunk)
        n_fail += 1
        continue

    p_open, p_close = find_params(sig)
    if p_open < 0:
        hdr_out.extend(chunk)
        n_fail += 1
        continue

    head_part = sig[:p_open]
    m = re.search(r'([~]?\w+)\s*$', head_part)
    if not m:
        hdr_out.extend(chunk)
        n_fail += 1
        continue
    name = m.group(1)
    ret = head_part[:m.start()].strip()
    params = sig[p_open + 1:p_close]
    suffix = sig[p_close + 1:].strip()

    if re.search(r'=\s*(default|delete|0)', suffix):
        hdr_out.extend(chunk)
        continue

    hdr_out.append(sig.rstrip() + ';')

    ret_cpp = ret
    for q in ('static ', 'virtual ', 'inline '):
        if ret_cpp.startswith(q):
            ret_cpp = ret_cpp[len(q):].strip()
    suffix_cpp = re.sub(r'\b(override|final)\b', '', suffix).strip()
    sig_cpp = (ret_cpp + (' ' if ret_cpp else '') + 'PyMfcHost::' + name + '(' + drop_defaults(params) + ')'
               + ((' ' + suffix_cpp) if suffix_cpp else ''))
    sig_cpp = qual_nested(sig_cpp)
    cpp_defs.append(sig_cpp + '\n' + body_txt)
    n_method += 1

new_hdr = '\n'.join(head + hdr_out + tail)
cpp = ('// PyMfcHost.cpp —— class PyMfcHost 的实现（从 PyMfcHost.h 类体内的内联定义迁出）。\n'
       '// 头里只留声明、嵌套类型、数据成员与宏。改实现改这里，改接口改 PyMfcHost.h。\n\n'
       '#include "PyMfcHost.h"\n\n' + '\n\n'.join(cpp_defs) + '\n')

io.open(os.path.join(d, 'PyMfcHost.h'), 'w', encoding='utf-8', newline='').write(new_hdr)
io.open(os.path.join(d, 'PyMfcHost.cpp'), 'w', encoding='utf-8-sig', newline='').write(cpp)
print('抽出方法 %d，解析失败保留 %d 段' % (n_method, n_fail))
print('PyMfcHost.h %d 行；PyMfcHost.cpp %d 行' % (len(new_hdr.split('\n')), len(cpp.split('\n'))))
