import json, pathlib, re

ROOT = pathlib.Path(r'G:\PyArx\PyMfcXml')
SRC = pathlib.Path(r'G:\PyArx\log\PyMfcHost.cpp.bak')
lines = SRC.read_text(encoding='utf-8').splitlines()

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

def blocks_of(seq, base=0):
    state = (False, None); depth = 0; begin = None; opened = False; res = []
    for idx, ln in enumerate(seq, 1):
        code, state = strip_code(ln, state)
        if begin is None and code.strip(): begin = idx
        for ch in code:
            if ch == '{': depth += 1; opened = True
            elif ch == '}':
                depth -= 1
                if depth <= 0:
                    depth = 0
                    if opened:
                        res.append((begin + base, idx + base)); begin = None; opened = False
        if depth == 0 and code.strip().endswith(';') and begin is not None:
            res.append((begin + base, idx + base)); begin = None; opened = False
    if begin is not None: res.append((begin + base, len(seq) + base))
    return res

allb = blocks_of(lines)
def head(b):
    s, e = b
    for i in range(s-1, e):
        if lines[i].strip(): return lines[i].strip()
    return ''
def text(b):
    return '\n'.join(lines[b[0]-1:b[1]])

def member_name(h):
    m = re.search(r'PyMfcHost::(~?\w+)\s*\(', h)
    return m.group(1) if m else None

RULES = [
 ('Procs', r'^(PyMfc\w*Proc|PyMfcTabMsgHook|ensureTabHook)$'),
 ('Paint', r'^(setSurface|moveSurfaces|getSurfaceRect|registerPaintNode|updatePaintNodeRect|findPaintNode|ancestorTransformOf|nodeXfOf|drawNode\w*|drawOwnerNode|hitNode|OnMouseMove|OnLButtonDown|OnLButtonUp|drawPaintNodes|drawContainerBackdrops|surfaceColorAt|drawDwgCanvas|drawRibbon|drawFileTab|drawView3D|repaintCanvas|applyNativeThemeColors)$'),
 ('Layout', r'^(attachXml|reflowXml|syncOneContainerBackdrop|syncContainerBackdrops|setXmlProperty|applyContainerBackdrop|ensureOverlay|hideOverlay|isOverlayVisible|placeOverlay|moveControl|moveMany|setVisible|isControlHidden|setEnabled|setLayerOpacity|getLayerOpacity|removeControlById|repairSiblingsOf|scrollOverflow\w*|registerStickyChild|setControlAppearance|applyTabProps|applyZOrder|applyLevelsAndTabs|setControlLevel|getControlLevel|setControlTabIndex|getControlTabIndex|setLevel|getLevel|maxLevel|minLevel|noteLevel|noteTabIndex|levelOf|tabIndexOf|bringToFront\w*|sendToBack|raiseMany|setTabIndex|getTabIndex|noteCreated|setPageBackdrop|setBackgroundColor)$'),
 ('Dwg', r'^(addDwgCanvas|fitDwgCanvas|setDwg\w*|getDwg\w*|copyDwgScene|dwgHit|dwgCardRect|selectDwgItem|detailHit|setDetailCard|clearDetailCard|fireDwg\w*|saveControlPng|addView3D|setView3D\w*|view3D\w*|addRibbon|setRibbonElement|fireRibbonButton|ribbonMouse|addFileTab|setFileTabElement|fireFileTab|fileTabMouse|getFileTabInfo|setImage|setImageFit|setImagePosition|loadBitmapFromFile)$'),
 ('Anim', r'^(animate\w*|stopAnimations|setBehavior|behaviorFor|animTick|animAdd|applyAnimValue|currentPropValue|easeAt|lerpColor|ensureAnimWnd|flick\w*|startCssKeyframes|startHoverTransition|pageTick|pageSlide|viewAnimate|stepSpin|bumpBusyPhase)$'),
 ('Interop', r'^(on[A-Z]\w*|onClick|fire\w*|getXmlValue|setWheelRegion|fireWheelMessage|perfStats)$'),
 ('HostBase', r'^(create|ensureSurface|setTheme|applyFont|hostHwnd|PostNcDestroy|ensureSkinBoxClass|skinOldProcs|isSkinPaintClass|requestSkinRepaint|isNativePaintClass|ensureWicCom|propInt|clearIdCache|registerChildId|findByIdTree|findByIdDeep|getDlgItemHandle|getHostWindow|controlClientSize|getControlRect|hostRectOfWindow|installDragBar|setFocusTo|clearCallbacks|caretBlink\w*|pyMfcNextTabItem)$'),
]

member_blocks = []
free_blocks = []
for b in allb:
    h = head(b)
    if h.startswith('#'):
        free_blocks.append(b); continue
    if member_name(h):
        member_blocks.append((member_name(h), b))
    else:
        free_blocks.append(b)

first_member_start = min(b[0] for _, b in member_blocks)
prologue_blocks = [b for b in free_blocks if b[0] < first_member_start]
rest_free = [b for b in free_blocks if b[0] >= first_member_start]

buckets = {}
for name, b in member_blocks:
    for bk, rx in RULES:
        if re.match(rx, name):
            buckets.setdefault(bk, []).append(b); break
    else:
        buckets.setdefault('Controls', []).append(b)

print('prologue_blocks', len(prologue_blocks), 'rest_free', len(rest_free), 'members', len(member_blocks))
for k in sorted(buckets): print(' ', k, len(buckets[k]))

# ---- build internal header declarations from prologue + rest_free
decls, defs = [], []

def add_construct(t):
    stripped = t.strip()
    if not stripped: return
    if stripped.startswith('#'):
        keep = []
        rest = []
        for ln in t.splitlines():
            if not rest and ln.strip().startswith('#'):
                keep.append(ln)
            else:
                rest.append(ln)
        if rest and any(x.strip() for x in rest):
            add_construct('\n'.join(rest))
        return
    if re.match(r'namespace', stripped):
        inner = re.sub(r'^\s*namespace\s*\{', '', t, count=1)
        inner = re.sub(r'\}\s*$', '', inner, count=1)
        ilines = inner.splitlines()
        for ib in blocks_of(ilines):
            add_construct('\n'.join(ilines[ib[0]-1:ib[1]]))
        return
    if re.match(r'(struct|class|enum|using|typedef)\b', stripped):
        decls.append(t.strip())
        return
    # drop leading comment lines so the signature stays on a real code line
    body_lines = t.splitlines()
    while body_lines and (not body_lines[0].strip() or body_lines[0].strip().startswith('//')):
        body_lines.pop(0)
    raw = '\n'.join(body_lines).strip()
    code = raw.split('{')[0].split(';')[0]
    sig = re.sub(r'^(?:(?:static|inline)\s+)+', '', code).strip()
    sig = ' '.join(sig.split())
    if '(' in sig:
        decls.append(sig + ';')
        first, rest = (body_lines[0], body_lines[1:]) if body_lines else ('', [])
        first = re.sub(r'^(?:(?:static|inline)\s+)+', '', first)
        defs.append('\n'.join([first] + rest))

for b in prologue_blocks + rest_free:
    add_construct(text(b))

hdr = ['// PyMfcHostInternal.h —— PyMfcHost 实现拆分后的内部共享声明（非公开接口）。',
       '#pragma once',
       '#include "PyMfcHost.h"',
       '#include "PyMfcPerf.h"',
       '#include "PyMfcDwgGL.h"',
       '#include "XmlSurface.h"',
       '#include "XmlGeom.h"',
       '#include <sstream>', '#include <cmath>', '#include <algorithm>',
       '']
seen = set(); uniq = []
for d in decls:
    k = ' '.join(d.split())
    if k in seen: continue
    seen.add(k); uniq.append(d)
hdr += uniq + ['']
(ROOT/'PyMfcHostInternal.h').write_text('\n'.join(hdr), encoding='utf-8')

base = ['// PyMfcHost.Base.cpp —— 公共前置、文件级辅助与静态回调（拆分自 PyMfcHost.cpp）。',
        '#include "PyMfcHostInternal.h"', '']
for d in defs:
    base.append(d); base.append('')
base.append('')
(ROOT/'PyMfcHost.Base.cpp').write_text('\n'.join(base), encoding='utf-8')

for bk, blks in buckets.items():
    out = [f'// PyMfcHost.{bk}.cpp —— 拆分自 PyMfcHost.cpp（按职责分组；接口声明仍在 PyMfcHost.h）。',
           '#include "PyMfcHostInternal.h"', '']
    for b in blks:
        out.append(text(b)); out.append('')
    (ROOT/f'PyMfcHost.{bk}.cpp').write_text('\n'.join(out), encoding='utf-8')

print('written header decls', len(uniq), 'defs', len(defs))
