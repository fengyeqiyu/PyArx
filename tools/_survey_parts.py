import io, re

def scan(lines):
    out=[]; state=None
    for line in lines:
        d=0; op=False; i=0
        while i < len(line):
            ch=line[i]; nx=line[i+1] if i+1<len(line) else ''
            if state=='lc': break
            if state=='bc':
                if ch=='*' and nx=='/': state=None; i+=2; continue
                i+=1; continue
            if state=='s':
                if ch=='\\': i+=2; continue
                if ch=='"': state=None
                i+=1; continue
            if state=='c':
                if ch=='\\': i+=2; continue
                if ch=="'": state=None
                i+=1; continue
            if ch=='/' and nx=='/': state='lc'; i+=2; continue
            if ch=='/' and nx=='*': state='bc'; i+=2; continue
            if ch=='"': state='s'; i+=1; continue
            if ch=="'": state='c'; i+=1; continue
            if ch=='{': d+=1; op=True
            elif ch=='}': d-=1
            i+=1
        if state=='lc': state=None
        out.append((d,op))
    return out

for name in ('04-wrap.inc','05-load.inc','06-bind.inc'):
    p = 'G:/PyArx/PyMfcXaml/parts/' + name
    lines = io.open(p, encoding='utf-8-sig').read().split('\n')
    sc = scan(lines)
    print('===== %s (%d 行) =====' % (name, len(lines)))
    depth = 0
    i = 0
    while i < len(lines):
        l = lines[i]
        if depth == 0 and l.strip():
            first = l.strip()
            if re.match(r'(namespace|class |struct |extern |using |typedef |#include)', first):
                print('%5d | %s' % (i+1, l[:110]))
            elif re.search(r'\w\s*\(', first) and not first.startswith('//'):
                print('%5d | FUNC %s' % (i+1, l[:110]))
        depth += sc[i][0]
        i += 1
    print()
