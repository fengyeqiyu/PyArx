sig = 'static std::map<HWND, bool>& caretBlinkStates()'
import re
p_open = sig.find('(')
head_part = sig[:p_open]
m = re.search(r'([~]?\w+)\s*$', head_part)
name = m.group(1)
ret = head_part[:m.start()].strip()
print('ret=%r name=%r' % (ret, name))
ret_cpp = ret
for q in ('static ', 'virtual ', 'inline '):
    if ret_cpp.startswith(q):
        ret_cpp = ret_cpp[len(q):].strip()
print('ret_cpp=%r' % ret_cpp)
