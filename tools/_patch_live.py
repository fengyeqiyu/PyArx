import io
p = r'G:\PyArx\UIDesign\DesignerPaneView.cpp'
s = io.open(p, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in s else '\n'

# 1) OnSize：改用库里的 LiveResize 判定
old_onsize = nl.join([
'    const ULONGLONG now = ::GetTickCount64();',
'    const bool continuous = (m_lastSizeTick != 0 && now - m_lastSizeTick < 120);',
'    m_lastSizeTick = now;',
'',
'    if (continuous || m_sizeMove)',
])
new_onsize = nl.join([
'    // 判据来自控件库（xmlframe::LiveResize）：连两次 WM_SIZE 间隔 <120ms 即正在拖。',
'    // 单次尺寸变化（还原、最大化、程序改尺寸）间隔大，走正常重排。',
'    const ULONGLONG now = ::GetTickCount64();',
'    const bool continuous = m_live.noteSize(now);',
'',
'    if (continuous || m_sizeMove)',
])
assert old_onsize in s, 'onsize pattern'
s = s.replace(old_onsize, new_onsize, 1)

# 2) OnTimer：静止判据交给库
old_timer = nl.join([
'        if (::GetTickCount64() - m_lastSizeTick < 200)',
'        {',
'            SetTimer(1, 60, nullptr);',
'            return;',
'        }',
])
new_timer = nl.join([
'        if (!m_live.settleDue(::GetTickCount64(), 200))',
'        {',
'            SetTimer(1, 60, nullptr);',
'            return;',
'        }',
])
assert old_timer in s, 'timer pattern'
s = s.replace(old_timer, new_timer, 1)

# 3) begin/end 同步库状态
s = s.replace('    m_sizeMove = true;' + nl,
              '    m_sizeMove = true;' + nl + '    m_live.begin();' + nl, 1)
s = s.replace('    m_sizeMove = false;' + nl,
              '    m_sizeMove = false;' + nl + '    m_live.end();' + nl, 1)

# 4) 收尾时把库缓冲作废（尺寸已变）
old_inv = '    m_backSize = CSize(0, 0);       // 尺寸已变，缓存位图作废，下一次全量重画'
new_inv = '    m_back.invalidate();            // 尺寸已变，库缓冲下次 ensure 时重建'
assert old_inv in s, 'invalidate pattern'
s = s.replace(old_inv, new_inv, 1)

io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('LiveResize wired')
