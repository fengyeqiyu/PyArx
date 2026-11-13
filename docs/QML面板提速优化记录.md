# QML 面板提速优化记录

日期：2026-11-10  
对象：`G:\PyArx\PyArxQml`（PyArxQml.pyd，在 AutoCAD 进程内承载 Qt Quick 面板）

---

## 一、结论

| 阶段 | 优化前（CAD 进程实测，4:21 日志） | 优化后（同等环境复测） |
|---|---|---|
| 应用实例构造 | **3410 ms**（`new QApplication`） | 279～300 ms（`new QGuiApplication`，含 11 个 DLL 预载） |
| 挂载 `Attach` | 约 133 ms（setSource 段） | 123～135 ms |
| `Attach` 返回 → 首帧 | **3707 ms** | **47～48 ms**（1 次 pump 即出帧） |
| 端到端（不含 import） | **7293 ms** | **452～484 ms** |

提速约 **15 倍**。剩余的 280 ms 主要是 Qt DLL 首次映射 + 平台插件 + 字体数据库初始化，属正常量级。

## 二、优化前的两个"假慢"来源

### 1. 首帧 3.7 秒是测试脚本自己饿死了消息泵

证据：`tests\qt_spike\pyarx_qml_demo.py` 第 43 行 `time.sleep(2.0)`、第 53 行 `time.sleep(1.5)`。

泵由 `WM_TIMER(16ms)` 驱动 `QCoreApplication::processEvents`，**Python 主线程 sleep 时没有消息循环，WM_TIMER 永远不派发**，一帧都出不来。时间线完全吻合：SetParent +3586ms → sleep 2.0s → 投点击 → sleep 1.5s → 到 +7293ms Python 一恢复，首帧立刻到。

**结论：这不是产品缺陷。测量/自检脚本必须用 `pump()` 轮询，不能 sleep。**

### 2. `QApplication` 3.4 秒只在 CAD 进程内发生

对照实验（同一台机、同一个 Qt prefix）：

| 程序 | 应用实例构造耗时 |
|---|---|
| 空进程 `QGuiApplication`（native exe） | **20 ms** |
| 空进程 `QApplication`（native exe） | **24 ms** |
| python 进程内 `QGuiApplication`（优化后） | 279～300 ms |
| **acad.exe 进程内 `QApplication`（优化前）** | **3410 ms** |

说明 Widgets 不是元凶（只多 4ms），慢在宿主进程上下文：CAD 是已初始化 COM/TSF 的 MFC 进程，QPA 平台插件初始化与其套间模型交互、加上字体与样式初始化被放大。

对应的解法是**根本不做第二次初始化**——复用宿主已有的 `QGuiApplication` 实例。

## 三、代码改动

### `PyArxQml.cpp`

1. `#include <QtWidgets/QApplication>` → `<QtGui/QGuiApplication>`；
   `static QApplication* s_app` → `static QGuiApplication* s_app`，新增 `static bool s_owned`。
   QML 面板只用 QQuickView，不依赖 Widgets，实测 Widgets 只值 4ms，去掉后少加载 `Qt6Widgets.dll`(5.6MB)。
2. `QmlHost::Ensure()` 增加**复用宿主实例**分支：
   ```cpp
   QCoreApplication* existing = QCoreApplication::instance();
   if (existing) {
       if (QGuiApplication* hostApp = qobject_cast<QGuiApplication*>(existing)) {
           s_app = hostApp;  s_owned = false;
           return EnsurePump();      // 跳过整套初始化
       }
       LogWrite("宿主已有 QCoreApplication 但非 QGuiApplication，无法复用");
   }
   ```
   宿主（如 CAD 自己已起 Qt）存在实例时直接接上，省掉全部构造成本。
3. 计时起点前移到 `ConfigEnv()` 之前，新增两段打点：`环境与预载完成`、`QGuiApplication 构造完成（构造本身耗时 …）`，并输出 `platformName()` 与 `QLibraryInfo::PluginsPath`，便于下次直接定位平台插件开销。
4. `PreloadQmlDeps()` 的清单里删掉 `Qt6SvgWidgets.dll`——该文件在本机与 CAD 目录都不存在，每次必然走完整失败搜索。

### `build_qml.bat`

- `set QT=G:\Qt\6.2.4\msvc2019_64` → `G:\Qt\6.2.6\msvc2019_64`
  （原路径**在本机并不存在**，脚本按原样跑必然失败；这是既有隐患，已修）
- 移除 `QtWidgets` 的 include 与 `Qt6Widgets.lib` 依赖。

## 四、验证结果

新编出的 `PyArxQml_new.pyd`（68KB，`/LD /MD /O2`，链接 CAD 的 `.def` 导入库）实测：

```
import:        10.4 ~ 14.3 ms
ensure():     279.3 ~ 300.3 ms      # QGuiApplication + 泵定时器
attach():     122.7 ~ 135.0 ms      # QQuickView + setSource + SetParent
首帧:          47.3 ~  48.3 ms      # 一次 pump() 即出帧
总计:         451.6 ~ 483.9 ms
```

测量方式：`docs\_qt_baseline\qml_timing3.py` —— ctypes 自建宿主窗口，调用
`PyArxQml.attach(qml, hwnd, 340, 620)`，然后**连续 `pump()` 轮询**而非 sleep。
这与 CAD 内的挂载路径等价。

同目录另有两支纯 Qt 基线程序（`baseline.exe` / `baseline_app.exe`），用于把"Qt 固有成本"与"宿主放大"分开：
`QGuiApplication` 254ms、`QApplication` 291ms 出首帧。

## 五、待完成的落地动作

**替换旧 pyd 需要关闭 AutoCAD 一次。**

原因：`acad.exe`（PID 53860，4:21:43 启动）正持有 `G:\PyArx\PyArxQml\PyArxQml.pyd`，
文件被锁定，编译无法直接覆盖（`LNK1104: 无法打开文件 PyArxQml.pyd`）。

关掉 CAD 后执行：

```powershell
Copy-Item G:\PyArx\PyArxQml\PyArxQml_new.pyd G:\PyArx\PyArxQml\PyArxQml.pyd -Force
Copy-Item G:\PyArx\PyArxQml\PyArxQml_new.pyd G:\PyArx\X64\Release\PyArxQml.pyd -Force
```

或在 CAD 关闭后直接跑 `G:\PyArx\PyArxQml\build_qml.bat`（已修好路径与依赖）。

**CAD 内还需复验一项**：`复用宿主已有 QGuiApplication` 分支是否真的命中。
日志出现 `复用宿主已有 QGuiApplication，跳过重复初始化 +x.x ms` 即为命中；
若打印 `宿主已有 QCoreApplication 但非 QGuiApplication`，说明 CAD 只起了 QCoreApplication，
此时仍需自建（3.4 秒无法完全消除，但可考虑把面板初始化延后到后台线程）。

## 六、后续还能做的（按收益）

1. **QML 预编译**：用 `G:\Qt\6.2.6\msvc2019_64\bin\qmlcachegen.exe` 把 `.qml` 编译成 `.qmlc`，
   可省掉运行期解析（当前 setSource 约 120ms 里的一部分）。
2. **首屏 import 精简**：`QtQuick.Controls` 冷加载最贵，能不用 Controls 的页面用基础类型。
3. **尺寸抖动**：宿主客户区 340x878 与视图 340x620 不一致，`PumpInner()` 每 tick 都触发一次 resize 判断，
   建议 Attach 时直接按客户区尺寸建视图。
4. 把 `pyarx_qml_demo.py` 里的两处 `time.sleep` 换成 `while PyArxQml.stats()['frames'] < N: PyArxQml.pump()`。

---

## 附：相关文件

| 文件 | 说明 |
|---|---|
| `G:\PyArx\PyArxQml\PyArxQml.cpp` | 已改：QGuiApplication + 复用宿主实例 + 细粒度打点 |
| `G:\PyArx\PyArxQml\build_qml.bat` | 已改：Qt 路径 6.2.6、去掉 Widgets |
| `G:\PyArx\PyArxQml\PyArxQml_new.pyd` | 新编产物，待替换 |
| `G:\PyArx\docs\_qt_baseline\qml_timing3.py` | 优化后计时脚本（无 sleep，纯 pump） |
| `G:\PyArx\docs\_qt_baseline\baseline.exe` | QGuiApplication 基线 |
| `G:\PyArx\docs\_qt_baseline\baseline_app.exe` | QApplication 基线 |
| `G:\PyArx\docs\_qt_qml_timing3.log` | 优化后实测日志 |
