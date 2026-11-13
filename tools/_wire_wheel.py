import io, os
d = r'G:\PyArx\PyMfcXaml'

# 1) PyMfcCommon.h 追加声明
p = os.path.join(d, 'PyMfcCommon.h')
s = io.open(p, encoding='utf-8').read()
if 'installWheelHost' not in s:
    s += '''
// 滚轮拦截：把顶层窗口的窗口过程换成 PyMfcWheelProc，并登记宿主指针与原过程。
// 定义在 PyMfcProcs.cpp；返回原窗口过程，供未命中注册区域的滚轮原样转回。
WNDPROC installWheelHost(HWND hwnd, void* host);
'''
    io.open(p, 'w', encoding='utf-8', newline='').write(s)

# 2) PyMfcProcs.cpp 在 PyMfcWheelProc 定义之后插入实现
p = os.path.join(d, 'PyMfcProcs.cpp')
lines = io.open(p, encoding='utf-8-sig').read().split('\n')
idx = next(i for i, l in enumerate(lines) if l.startswith('static LRESULT CALLBACK PyMfcWheelProc(HWND hwnd'))
# 找到该函数体结束
depth = 0
started = False
end = idx
for i in range(idx, len(lines)):
    depth += lines[i].count('{') - lines[i].count('}')
    if '{' in lines[i]: started = True
    if started and depth == 0:
        end = i
        break
impl = '''
// 供其它编译单元使用：安装滚轮拦截（顶层窗口专用）
WNDPROC installWheelHost(HWND hwnd, void* host)
{
    ::SetPropW(hwnd, kWheelHostProp, reinterpret_cast<HANDLE>(host));
    WNDPROC old = reinterpret_cast<WNDPROC>(
        ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PyMfcWheelProc)));
    wheelOldProcs()[hwwnd_placeholder] = old;
    return old;
}'''
impl = impl.replace('hwwnd_placeholder', 'hwnd').split('\n')
lines[end + 1:end + 1] = impl
io.open(p, 'w', encoding='utf-8-sig', newline='').write('\n'.join(lines))

# 3) 04-wrap 改用 installWheelHost
p = os.path.join(d, 'parts', '04-wrap.inc')
s = io.open(p, encoding='utf-8-sig').read()
old = '''        ::SetPropW(hwnd, kWheelHostProp, reinterpret_cast<HANDLE>(&m_host));
        wheelOldProcs()[hwnd] = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PyMfcWheelProc)));'''
new = '''        installWheelHost(hwnd, &m_host);'''
assert old in s, '未找到滚轮安装段'
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)
print('已接线：installWheelHost')
