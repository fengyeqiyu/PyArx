import io
p = 'G:/PyArx/MfcXamlRuntime/runtime.py'
s = io.open(p, encoding='utf-8').read()
old = """        rel = M.xamlLayout(self.xaml, w, h)
        for name, (rx, ry, rw, rh) in rel.items():
            cid = self.ids.get(name)
            if cid is not None:
                self.win.moveMany(cid, x + int(rx), y + int(ry), int(rw), int(rh))
                self.rects[name] = (x + int(rx), y + int(ry), int(rw), int(rh))"""
new = """        rel = M.xamlLayout(self.xaml, w, h)
        batch = []
        for name, (rx, ry, rw, rh) in rel.items():
            cid = self.ids.get(name)
            if cid is not None:
                ax, ay = x + int(rx), y + int(ry)
                batch.append((cid, ax, ay, int(rw), int(rh)))
                self.rects[name] = (ax, ay, int(rw), int(rh))
        if batch:
            self.win.moveMany(batch)      # 一次搬完，不逐个 move（40 个逐个 move 会掉帧）"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('runtime.move 改成批量 moveMany')
