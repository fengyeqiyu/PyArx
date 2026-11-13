# plugins：XAML 组合出来的复合控件

## 一、机制

1. **MFCXaml 只提供控件库**：`Button` / `Label` / `TextField` / `ListCtrl` / `ListBox` / `Pane` /
   `Divider` / `Image` 这些基础控件由 C++ 侧创建与绘制，XAML 只管结构与尺寸。
2. **复合控件 = 基础控件 + XAML 布局**，放在本目录，一个 `.xaml` 一个复合控件：
   根 `<XWindow Name="...">` 是控件类型名，里面的 `x:Name` 是子控件名。
   几何全部由 `M.xamlLayout(path, w, h)` 算出，加载方不写任何像素计算。
3. **回调写在 XAML 里**：
   ```xml
   <XButton Name="BtnApply" Text="应用" on_click="on_apply"/>
   ```
   Python 侧只需定义同名函数（`def on_apply(comp, name)`），加载时按名字在调用方作用域里解析并绑定。
4. **import 即建对象**：`pp_plugin.import_plugin('side_panel', app, x, y, w, h)` 会
   - 在 plugin 目录里自动生成 `side_panel.py` 薄壳（`create(app,x,y,w,h,scope)`）；
   - 导入并调用它，返回带 `.ids / .rects / .set_text / .set_rows / .move` 的复合控件对象。

## 二、现有复合控件

| 文件 | 控件 | 子控件 | XAML 声明的回调 |
|---|---|---|---|
| `side_panel.xaml` | 编辑侧板 | `Title` / `Table`（ListCtrl）/ `BtnApply` / `BtnLoad` / `BtnClose` | `on_click="on_apply"` `on_load` `on_close` |

## 三、什么时候把它编进 C++

XAML 组合先跑通行为，**满足下面任一条就提升为 C++ 控件类**（在 `MfcXamlCore\controls\` 加一个类并注册）：

- 内部有**真实状态机**：拖动分割条、单元格编辑态、滚动虚拟化、展开/折叠动画。XAML 组合只能摆位置，
  状态还是要 Python 写，一写就变成"Python 拼界面"，失去意义。
- 有**自己的绘制**：阴影卡片、缩略位图、自绘表头。这些用基础控件拼不出来。
- **跨项目复用**：两处以上地方用同一个组合（如 `TableView` / `SplitPane`）。
- 已经进入**热路径**：每次数据变化都要重建子控件（如表格 39 行 × 8 列）。

提升时接口不变：XAML 里仍是同一个标签名（`<XTableView .../>`），属性与 `on_*` 回调名保持不变，
C++ 类内部接管创建与事件，Python 侧代码不用改。这样"XAML 编写 → 需要时下沉 C++"是一条可逆的路。

## 四、加载与验证

```python
import pp_plugin

# 1) 直接对象图：`child.click = fn` 绑事件
c = pp_plugin.xamlload(r'G:\PyArx\plugins\side_panel.xaml', app=app, x=10, y=10, w=640, h=380)
c.BtnApply.click = myclass.on_click        # XAML 里不需要写 on_click
c.Table.grid     = myclass.on_grid         # 表格按键：cb(action, row, col, text)
c.set_rows('Table', rows)                  # 一次灌满，不逐行插
c.move(20, 20, 500, 300)                   # 改外框尺寸，内部按 XAML 重排

# 2) XAML 预先声明回调，Python 只定义同名函数
#    <XButton Name="BtnApply" Text="应用" on_click="on_apply"/>
c = pp_plugin.import_plugin('side_panel', app, 10, 10, 640, 380)
def on_apply(comp, name): ...
```

两种写法等价，可混用：XAML 声明的在加载时解析，`child.click = fn` 在运行期覆盖/追加。

验证脚本：`tests\mfc_xaml\pp_plugin_test.py`（XAML 定位 / XAML 声明回调 / import 建对象 / 批量灌数据 / 重排）。
