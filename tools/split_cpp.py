import json, pathlib, sys

def strip_code(line, state):
    out = []; i = 0; n = len(line); inblk, instr = state
    while i < n:
        c = line[i]
        if inblk:
            j = line.find('*/', i)
            if j < 0: i = n; break
            inblk = False; i = j + 2; continue
        if instr:
            if c == '\\': i += 2; continue
            if c == instr: instr = None
            i += 1; continue
        if c == '"': instr = '"'; out.append(' '); i += 1; continue
        if c == "'": instr = "'"; out.append(' '); i += 1; continue
        if c == '/' and i + 1 < n and line[i+1] == '/': break
        if c == '/' and i + 1 < n and line[i+1] == '*': inblk = True; i += 2; continue
        out.append(c); i += 1
    return ''.join(out), (inblk, instr)

def blocks(path):
    txt = pathlib.Path(path).read_text(encoding='utf-8', errors='replace').splitlines()
    state = (False, None); depth = 0
    begin = None; opened = False
    res = []
    for idx, ln in enumerate(txt, 1):
        code, state = strip_code(ln, state)
        if begin is None and code.strip():
            begin = idx
        for ch in code:
            if ch == '{': depth += 1; opened = True
            elif ch == '}':
                depth -= 1
                if depth <= 0:
                    depth = 0
                    if opened:
                        res.append((begin, idx)); begin = None; opened = False
        if depth == 0 and code.strip().endswith(';') and begin is not None:
            res.append((begin, idx)); begin = None; opened = False
    if begin is not None:
        res.append((begin, len(txt)))
    return res, txt

if __name__ == '__main__':
    p = sys.argv[1]
    res, txt = blocks(p)
    print(f'{p}: blocks={len(res)}')
    for s, e in res:
        first = next((txt[i].strip() for i in range(s-1, e) if txt[i].strip()), '')
        print(f'{s:6d}-{e:6d} L{e-s+1:5d}  {first[:96]}')
    json.dump(res, open(p + '.blocks.json', 'w'), indent=0)
