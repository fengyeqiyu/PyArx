# MfcXamlRuntime —— 复合控件运行机制框架

C++ 只提供基础控件；复合控件由 XAML 组合而成，放在 `plugins\`；运行机制（加载 / 注册 / 事件 / 生命周期）在这一个包里。
通用组合需要更高性能时，把同一个 `kind` 的 factory 换成 C++ 侧控件即可，XAML 与 Python 代码不用改。

## 包结构

```
G:\PyArx\MfcXamlRuntime\
  __init__.py     导出 xamlload / Runtime / Composite / Child / import_plugin / available
  runtime.py      全部实现（Composite 对象图、Child 事件代理、Runtime 注册表、_dispatch 回调解析）
G:\PyArx\plugins\
  side_panel.xaml 复合控件定义（XAML Grid 定位 + on_* 回调声明）
  README.md       plugin 机制与「何时下沉 C++」
G:\PyArx\tests\mfc_xaml\pp_plugin.py   兼容转发到本包（老脚本不用改）
```

## 用法

```python
from MfcXamlRuntime import Runtime, xamlload

c = xamlload(r'G:\PyArx\plugins\side_panel.xaml', app=app, x=10, y=10, w=640, h=380)
c.BtnApply.click = myclass.on_click     # 运行期绑（对象图赋值）
c.Table.grid     = myclass.on_grid      # (action, row, col, text)

c.set_rows('Table', rows)               # 一次性灌满，不逐行插
c.set_text('Title', u'共 39 行')
c.move(20, 20, 500, 300)                # 改外框 → 内部按 XAML 重新算矩形，批量 moveMany
c.destroy()                             # 销毁子控件

kinds = Runtime.list()                  # 注册表 + plugin 目录扫描
c2 = Runtime.create('side_panel', app, 0, 0, 640, 380)
```

XAML 里也可以预先声明回调，加载时按函数名在调用方作用域解析（`app` 对象的方法也算）：

```xml
<Button Name="BtnApply" Text="应用" on_click="on_apply"/>
<Button Name="BtnLoad"  Text="载入" on_click="on_load"/>
```

回调签名自适应：`(comp, name, payload)` / `(comp, name)` / `(name)` / `()` 都能挂。

## 为什么几何全在 XAML

`Composite._build` 只做两件事：`M.xamlLayout(path, w, h)` 拿每个 `x:Name` 的矩形 → 按矩形建基础控件。
Python 侧没有一处像素常量；改布局 = 改 XAML。`move()` 同样是"换一个外框重新问一遍 XAML"。

## 已知问题

- ~~XamlCore 星号列取整丢 1px~~ **已修**：`ResolveTracks` 把取整余数补给最后一个 star 轨道，
  640 宽 `12 + 3*star + 12` 现在正好 640（三段 205/205/206）。
- ~~点击 → XAML 声明回调未验通~~ **已验证**（`pp_click_test` 5/5）：真实点击能同时调到
  "XAML 里声明的 `on_click`" 和"属性赋值的 `child.click`"。
- **坑（已修）**：XAML 声明的名字在当前作用域找不到时，原实现抛 `NameError`，会把同一控件上
  属性赋值的回调一起挡掉。现在解析失败只 `stderr` 提示一次并跳过，其余回调照常执行；
  作用域也改成取调用方模块的 `globals`（脚本以 `__main__` 跑也能解析到）。

## 验证

`tests\mfc_xaml\pp_runtime_test.py` —— 9 项：插件枚举、注册表、兼容转发、`Runtime.create`、XAML 定位
（`Table=(10,50,639,306)`）、`child.click` 绑定、XAML 声明 `on_click` 登记、批量灌 39 行、外框重排
（`(20,60,498,226)`）。当前 **9/9 PASS**。
