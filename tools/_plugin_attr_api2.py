import io
p = 'G:/PyArx/tests/mfc_xaml/pp_plugin.py'
s = io.open(p, encoding='utf-8').read()
old = """    def __repr__(self):
        return '<Child %s id=%s rect=%s>' % (self._name, self.id, self.rect)"""
new = """    # mybutton.click = fn 这样直接绑；每个事件一个可写属性
    click = property(lambda self: self._comp._cb.get((self._name, 'click'), None),
                     lambda self, fn: self._set_event('click', fn))
    dblclick = property(lambda self: self._comp._cb.get((self._name, 'doubleclick'), None),
                        lambda self, fn: self._set_event('doubleclick', fn))
    change = property(lambda self: self._comp._cb.get((self._name, 'change'), None),
                      lambda self, fn: self._set_event('change', fn))
    grid = property(lambda self: self._comp._cb.get((self._name, 'grid'), None),
                    lambda self, fn: self._set_event('grid', fn))

    def __repr__(self):
        return '<Child %s id=%s rect=%s>' % (self._name, self.id, self.rect)"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('Child.click 等可写属性已加')
