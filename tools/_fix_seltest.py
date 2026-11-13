import io
P = r'G:\PyArx\tests\mfc_xaml\pp_select_test.py'
s = io.open(P, encoding='utf-8').read()
s = s.replace("rc = app.win.planCardRect(pp_app.CV, code0)", "rc = app.win.cardRectPlan(pp_app.CV, code0)")
s = s.replace("if rc and rc[2] > rc[0]:", "if rc and len(rc) == 4 and rc[2] > 0:")
s = s.replace("cx, cy = (rc[0] + rc[2]) // 2, (rc[1] + rc[3]) // 2",
              "cx, cy = int(rc[0] + rc[2] / 2), int(rc[1] + rc[3] / 2)")
io.open(P,'w',encoding='utf-8').write(s)
print('ok')
