# CAD 侧加载与运行（窗体 / 侧栏）

本目录是「ARX 编译 + CAD 内加载 Python 脚本 + 界面以窗体与侧栏两种方式运行」的可复现记录。

## 1. 编译 ARX

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' `
  'G:\PyArx\PyArxEngine\PyArxEngine.vcxproj' `
  /p:Configuration=Release /p:Platform=x64 /m /v:m /nologo > 'G:\PyArx\build_arx1.log' 2>&1
```

产物：`G:\PyArx\X64\Release\PyArxEngine.arx`（同时输出同名 `.crx` / `.dll`）。
本次 `build_arx1.log` 统计 `: error ` 条数 = 0；源码未变动时 MSBuild 判定工程已最新，只做校验不重新链接。

## 2. 用 COM 驱动 CAD（无需重启）

AutoCAD 2024 常驻一个实例，Python 解释器由 ARX 首次命令时懒初始化并**一直存活**（软卸载），
后续换脚本只需再发一次 `PYRUNFILE`，不需要重启 CAD。

- `run_com.ps1`：创建/复用实例 → 打开文档 → `arxload` → `PYRUNFILE <一个脚本>`
- `run_both.ps1`：同一次会话里 `arxload` 一次，然后依次 `PYRUNFILE` 窗体脚本与侧栏脚本

```powershell
& 'G:\PyArx\tests\cad\run_both.ps1'
```

命令对照（ARX 注册，见 `PyArxEngine\acrxEntryPoint.cpp`）：
`PYLOAD` / `PYRUN` / `PYRUNFILE` / `PYEXEC` / `PIPM` / `PYDEBUG` / `PYARXDEPLOY`。
`PYRUNFILE` 无参数时会用 `acedGetString` 向命令行索要脚本路径，COM 里直接给下一行即可。

## 3. 两条脚本

| 脚本 | 入口 | 形态 | 结果 |
| --- | --- | --- | --- |
| `cad_form_test.py` + `cad_ui.xaml` | `create_embedded(CAD 主窗口 HWND)` + `guiload(target=w)` | 独立顶层窗体（`PyMfcNativeWindow`，540×460） | 8 个控件：TextBlock / 2×Button / CheckBox / ProgressBar(65%) / TextBox / GridView(12 格，选中 4) / QCanvas(5 种画命令) |
| `cad_palette_test.py` + `cad_ui_palette.xaml` | `PyArxCore.Ap.getPalette(...)` + `guiload(palette=pal)` | 左侧停靠侧栏（`CAdUiPaletteSet`，标题 `PY-MFC-PALETTE`） | 7 个控件：TextBlock / 2×Button / CheckBox / ProgressBar(65%) / TextBox / QCanvas |

侧栏 XAML 的根节点必须是 `<PaletteSet>`，页是 `<Page Header="...">`；页内走的是 flow（纵向堆叠）布局，
每个控件必须写 `x:Name`，否则 `guiload` 直接抛 `需要 x:Name 属性`。
根节点必须声明 `xmlns:x`，否则 XML 解析阶段就会因 `x:Name` 前缀未绑定而失败（`unbound prefix`）。

## 4. 取证

| 文件 | 内容 |
| --- | --- |
| `docs\cad_both_shot.png` | CAD 主窗口：左侧 `PY-MFC-PALETTE` 侧栏已停靠并画出全部控件，命令行显示两次 `PYRUN` 均 `rc=0` |
| `docs\cad_form_win.png` | 窗体方式：`PyArx CAD 窗体测试` 独立窗口，GridView 12 格（第 4 格高亮）、QCanvas 图形齐全 |

抓图用 `shot_win.py`：父窗口 `PrintWindow` + 每个子窗口 `WM_PRINTCLIENT` 离屏合成，
因此 D2D 子控件（GridView / QCanvas / ProgressBar）也能取到真实像素。

## 5. 脚本写法上的两条硬规则（踩过坑才稳定）

1. **窗口必须自己留引用**：`PYRUNFILE` 执行完，脚本模块里的 `w` 一被回收，窗口就跟着没用的人
   （用户看到"窗口一闪就没"）。统一用 `_cadkeep.keep(w)` 挂到 `sys` 上：
   ```python
   import sys as _sys
   _sys.path.insert(0, r'G:\PyArx\tests\cad')
   from _cadkeep import keep as _keep
   w = _keep(PyMfcCore.create_embedded(parent, "标题", 540, 460))
   ```
2. **不要指望 COM 里只有一个 CAD**：AutoCAD 会额外拉起无主窗口的 `acad.exe`，
   `GetObject('','AutoCAD.Application')` 可能附着到那个头上，`PYRUNFILE` 发过去被静默忽略
   （表现：脚本 log 一字节没增）。`run_all_in_cad.ps1` 的做法是：先结束全部 `acad`，
   用 COM 起一个，再**用 `pyengine.log` 是否新增**确认 `arxload` 落到这个实例上，
   之后每个脚本都用各自 log 的字节数增量判断真的执行了。

窗口显示时机：`create_embedded` 建窗口时是隐藏的，`guiload` 把控件树建完才一次性
`ShowWindow` + 全量重画（`PyMfcCore.cpp` 的 `markSilentBuild/endSilentBuild`）。
所以抓图前不需要等"补画"，窗口第一次出现就是完整的。

兄弟窗口互不重叠：动画/程序改某个控件的 X/Y/Width/Height 后，`PyMfcHost::repairSiblingsOf`
会把与"旧矩形 ∪ 新矩形"相交的兄弟控件强制重画一次（`RDW_INVALIDATE|RDW_UPDATENOW`），
否则被它压过、后来又让出来的那块会留下空洞——表现就是"控件时有时无"。

页容器切页：`addPage` 记录的是每个页内控件的**完整基准矩形**，切页只平移不改尺寸，
切回来就是最初的样子；同一页内按声明逆序摆窗，保证"后声明的压在上面"。
点页码条左/右三分之一就是上一页/下一页，横向拖动超过 40px 也翻页。

Flickable 视口：`w.setFlickContent(fid, [kid, ...], contentW, contentH)` 把页内子控件
搬到视口窗里（子窗天然被父窗裁剪，所以超出的部分不会画到视口外），
拖拽 1:1 跟随（越界部分按 0.5 阻尼）、松手按速度惯性衰减（`v *= 0.92`）、
越界后每帧按 0.86 收敛回边界；`setFlickOffset` / `getFlickOffsetX/Y` 可脚本驱动。
搬进来的控件仍走原来的宿主绘制（`WM_DRAWITEM`/`WM_CTLCOLOR*` 由视口窗转给宿主）。
