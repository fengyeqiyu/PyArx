import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, ppApp
app = ppApp.App(); app.win.show(); M.pump(200)
w = app.win
w.selectPlan(ppApp.CV, 'AT1-1'); app.onCanvasClick(); M.pump(40)
print('curSel', app.curSel, 'DT_TITLE', w.getText(ppApp.DT_TITLE))
print('D_F0..3 =', [w.getText(ppApp.D_F0+i) for i in range(4)])
w.setText(ppApp.D_F0+1, '1号柜1号回路-详情改'); M.pump(30)
print('setText 后 D_F0+1 =', repr(w.getText(ppApp.D_F0+1)))
print('_detailValues =', app._detailValues())
app.onDetailSave(); M.pump(40)
print('save 后 name =', repr((app.nodeOf('AT1-1') or {}).get('name')))
before = len(app.nodes); app.onDetailAddchild(); M.pump(30)
print('addchild', before, '->', len(app.nodes), 'curSel', app.curSel)
os._exit(0)
