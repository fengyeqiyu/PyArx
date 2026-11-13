import io
p=r'G:\PyArx\docs\mfcxaml-render-refactor-plan.md'
add = r'''

## 2026-11-11 追加：底层换框架（用户指令"继续重构，使用更新的框架，从底层修改"）

### 决策
不再在 GDI/U-控件的既有骨架上打补丁，改为在 **MfcXamlCore 底层**换渲染框架：
- **D3D11 设备 + DXGI 交换链（FLIP_DISCARD, 2 缓冲, B8G8R8A8）+ ID2D1DeviceContext**，
  取代当前的 `ID2D1DCRenderTarget::BindDC` + 每控件 mem DIB + `BitBlt` 三层结构。
- **单一表面**：宿主窗口只有一个渲染目标；自绘控件不再是 HWND，而是保留树里的绘制节点。
- **原生 HWND 只保留真正需要输入的**（Edit、滚动条等），作为覆盖层。

### 分层施工（每层独立可编译、可回退）
1. **S1 设备层**：新增 `XamlSurface`（MfcXamlCore），负责 D3D11 设备、swap chain、D2D DeviceContext、DWrite；
   处理设备丢失（`D2DERR_RECREATE_TARGET` / `DXGI_ERROR_DEVICE_REMOVED`）重建。
2. **S2 表面层**：`PyMfcHost` 的 WM_PAINT 改走 `XamlSurface::BeginFrame/EndFrame`；
   各控件的绘制改为"注册绘制节点（rect + z + draw 回调）"，由表面按 z 序一次画完。
3. **S3 控件去 HWND**：STATIC owner-draw 全部转 scene node；Edit/滚动条保留覆盖层。
4. **S4 布局与重排分离**：resize 只标脏，泵内统一 layout+draw，消灭"WM_SIZE 里同步重排"。
5. **S5 表格 owner-data**：只灌可见行。

### 兼容与回退
环境变量 `PYARX_SURFACE`：`1`=新表面路径，`0`=旧 GDI 路径。默认先 1、可随时切回 0 做对照。
`PYARX_D2D_RT=software/hardware` 保留（仅影响旧路径）。

### 验收
- 拖动分割线 ≤60 ms/步（当前 34.4 ms，达标，不得回退）
- 详情弹窗 ≤40 ms（当前 65 ms）
- 画布平移无闪烁；表格滚动无闪烁
- 118 项回归绿
'''
with io.open(p,'a',encoding='utf-8') as f: f.write(add)
print('plan.md 已追加底层换框架章节')
