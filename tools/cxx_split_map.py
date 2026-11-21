import re, sys, json, pathlib

CTRL = {'if','for','while','switch','catch','else','do','return','case','sizeof',
        'static_assert','namespace','typedef','using','class','struct','enum','union','extern'}

def strip_comments(lines):
    out=[]; inblk=False; instr=None
    for ln in lines:
        s=''; i=0
        while i < len(ln):
            c=ln[i]
            if inblk:
                j=ln.find('*/', i)
                if j<0: i=len(ln); break
                inblk=False; i=j+2; continue
            if instr:
                s+=c
                if c=='\\': 
                    if i+1<len(ln): s+=ln[i+1]
                    i+=2; continue
                if c==instr: instr=None
                i+=1; continue
            if c=='"': instr='"'; s+=c; i+=1; continue
            if c=="'": instr="'"; s+=c; i+=1; continue
            j=ln.find('/*', i); k=ln.find('//', i)
            if k>=0 and (j<0 or k<j): s+=ln[i:k]; i=len(ln); break
            if j>=0: s+=ln[i:j]; inblk=True; i=j+2; continue
            s+=ln[i:]; i=len(ln)
        out.append(s)
    return out

NAME = re.compile(r'([A-Za-z_~][\w:~]*)\s*\(')

def kind_of(header):
    h = ' '.join(header.split())
    if re.search(r'\bnamespace\b', h): return 'ns'
    if re.search(r'\b(class|struct|union|enum)\b', h) and '(' not in h: return 'cls'
    if '(' in h: return 'fn'
    return 'blk'

def map_file(path):
    raw = pathlib.Path(path).read_text(encoding='utf-8', errors='replace').splitlines()
    cl = strip_comments(raw)
    funcs=[]; stack=[]; pending=''; pending_line=0
    for idx, ln in enumerate(cl, 1):
        for ch in ln:
            if ch=='{':
                kind = kind_of(pending)
                if kind=='fn':
                    m = NAME.search(' '.join(pending.split()))
                    name = m.group(1) if m else '?'
                    if name.split('::')[-1] not in CTRL:
                        funcs.append({'name':name,'start':pending_line or idx})
                        stack.append(['fn', idx])
                    else:
                        stack.append(['blk', idx])
                else:
                    stack.append([kind, idx])
                pending=''
            elif ch=='}':
                if stack:
                    k, st = stack.pop()
                    if k=='fn':
                        funcs[-1]['end']=idx
                pending=''
            elif ch==';':
                pending=''
            else:
                pending += ch
        pending += ' '
        if pending.strip()=='' : pending_line=0
        elif pending_line==0: pending_line=idx
    return funcs, len(raw)

for p in sys.argv[1:]:
    funcs,total = map_file(p)
    print(f'== {p}  lines={total}  funcs={len(funcs)}')
    for f in funcs:
        print(f"{f['start']:6d}-{f.get('end',total):6d} L{f.get('end',total)-f['start']+1:5d}  {f['name']}")
    json.dump({'file':p,'lines':total,'funcs':funcs}, open(p+'.map.json','w',encoding='utf-8'), ensure_ascii=False, indent=0)
