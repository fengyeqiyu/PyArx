# CAD 进程内 QGuiApplication 3.2s 定位记录

**结论**：代价不在 Qt 初始化，也不在插件加载，而是 **Qt 两次扫描 AutoCAD 程序目录（439 个 DLL、1005 MB）**，每次约 2 秒，合计 3.2~4.3 秒。这个目录是 Qt 自己无条件加进库搜索路径的（`appendApplicationPathToLibraryPaths()`），插件侧无法移除。**唯一可行做法是把这 3 秒挪到不挡人的位置（预热）**，已实现并实测：预热 3.5s，之后开面板只要 **0.6s**。

---

## 一、实测数据（AutoCAD 2024 进程内，CHENHUI-WPC）

| 场景 | 构造本身 | 总 |
|---|---|---|
| `ensure_core()`：只建 QCoreApplication | **3.3 ms** | 21.9 ms |
| `ensure()`：QGuiApplication，CAD 自带 qwindows | 3538.3 ms | 3553.4 ms |
| `ensure()`：QGuiApplication，自备 `X64\Release\platforms\qwindows.dll` | 3222.4 ms | 3236.8 ms |
| `ensure()`：QGuiApplication，`fontengine=freetype` | 3222.8 ms | 3237.2 ms |
| `ensure()` + 开 `QT_DEBUG_PLUGINS`（同一台机器、同一时段） | **4315.3 ms** | 4332.0 ms |
| python 独立进程 `ensure()`（同一份 pyd） | 319 ms | — |
| python 独立进程，只有 `X64\Release` 在 PATH（自包含） | 321.7 ms | 330.0 ms |

同一份二进制，CAD 里 3~4s、python 里 0.32s，差 10 倍以上 —— 代价来自宿主进程环境。

## 二、决定性证据：Qt 在扫 AutoCAD 程序目录

`QT_DEBUG_PLUGINS=1` + `PYARXQML_QPA_LOG=1` 打开后，三行日志说明一切：

```
14:47:35.590  [qt] QFactoryLoader::QFactoryLoader() checking directory path
                    "C:/Program Files/Autodesk/AutoCAD 2024" ...
14:47:37.663  [qt] ... checking ".../Plugins/platforms/platforms"     ← 前一扫用了 2.073 秒
...
14:47:37.672  [qt] checking "C:/Program Files/Autodesk/AutoCAD 2024" ...
14:47:39.790  [qt] ... checking ".../platformthemes"                  ← 又扫 2.118 秒
14:47:39.895  QGuiApplication 构造完成 +4332.0 ms
```

两次扫描，对象都是 CAD 根目录：

- 目录规模实测：**439 个 DLL，合计 1005.1 MB**（根目录共 768 个文件）
- 扫描内容：`QFactoryLoader` 逐个读每个 DLL 的 PE 插件元数据
- 两次 iid 分别是 `platforms`（平台插件）和 `platformthemes`（平台主题）

而真正的插件加载本身并不慢，同一次运行里我们自己的打点为：

```
诊断: LoadLibraryW(qwindows.dll) 耗时 1.0 ms（成功）
诊断: 读取 qwindows 插件元数据耗时 1.0 ms（键 6 个）
```

**单文件 1~2ms × 439 个文件 ≈ 2 秒**，与两段空档吻合。python 独立进程之所以快，是因为它的 app 目录（`D:\Program Files\Python314`）只有几十个文件。

## 三、Qt 源码确认：app 目录无法排除

`qtbase\src\corelib\kernel\qcoreapplication.cpp:568`

```cpp
void QCoreApplicationPrivate::appendApplicationPathToLibraryPaths()
{
    QString app_location = QCoreApplication::applicationFilePath();
    app_location.truncate(app_location.lastIndexOf(QLatin1Char('/')));
    app_location = QDir(app_location).canonicalPath();
    if (QFile::exists(app_location) && !app_libpaths->contains(app_location))
        app_libpaths->append(app_location);   // 无条件追加，没有开关
}
```

即使提前用 `setLibraryPaths()` 钉死单目录，构造过程仍会把这个 app 目录追加回来（构造后打印的路径就是 3 条）。所以 **CAD 场景下这 2×2 秒没有配置层面的解**；要么接受，要么预热。

## 四、逐个实测排除的假设（别再试）

| 假设 | 实测 | 结论 |
|---|---|---|
| 平台插件被 CAD 的 qwindows 劫持 | 换自备 `platforms\qwindows.dll` | 3538 → 3222，**不是根因** |
| 库搜索路径里塞了 CAD 目录导致扫描 | `setLibraryPaths()` 钉死单目录 | 日志确认已钉死，仍扫 app 目录，**无效** |
| 字体引擎（DirectWrite）建库慢 | `windows:fontengine=freetype` | 3222.8，**无关** |
| 字体数量 / 屏幕枚举 | `diag()`：440 个字体 8~31ms、2 块屏 0ms | **无关** |
| Qt 核心初始化 | `ensure_core()` | 3.3ms，**无关** |
| 插件 DLL 加载/解析慢 | `LoadLibraryW` 1.0ms、`QPluginLoader::metaData()` 1.0ms | **无关**，慢在"扫目录" |
| CAD 的 `plugins\` 目录太大 | 实测只有 9 个文件 2.3MB | 扫描对象是**根目录**，不是 plugins |
| QML 内容或 import 量 | baseline.qml(1 个 Rectangle) vs panel_demo.qml | 只影响 setSource（python 侧 78ms vs 490ms），不影响构造 |
| 用 minimal 平台绕开 | `QT_QPA_PLATFORM=minimal` | **直接把 CAD 崩溃**（RPC 0x800706BE）。Qt Quick 不能走合成平台，别试 |

## 五、已落地的实现

1. **QPA 目录优先级链**（`ConfigEnv`）：`PYARXQML_QPA_DIR` > `SelfDir\platforms` > `SelfDir\plugins\platforms` > CAD `Plugins\platforms` > SDK `plugins\platforms`（SDK 放最后，它的父目录会被 Qt 一并加入搜索路径，`setSource` 从 ~120ms 涨到 ~500ms）。
2. **`QCoreApplication::setLibraryPaths()`** 构造前钉死单目录（对构造无效，但能避免后续 `setSource` 扫 SDK plugins 根）。
3. **`QT_PLUGIN_PATH` 默认不设**，仅 `PYARXQML_PLUGIN_PATH` 显式指定时打开（一旦设了，`setSource` 从 123ms 涨到 775ms）。
4. **Qt 版本闸**：`qVersion()` 与编译期 `QT_VERSION_STR` 不一致则拒绝启动并写日志；`PYARXQML_ALLOW_QT_MISMATCH=1` 放行。CAD 自带 Qt 也是 6.2.6，实测通过。
5. **诊断接口**：`CoreOnlyProbe()`（只建 Core）、`diag()`（版本/平台/字体/屏幕）、`PYARXQML_QPA_LOG=1`（把 Qt 的 qpa 日志接到同一份日志），以及 `ConfigEnv` 里对 `LoadLibrary`/`metadata` 的计时。
6. ~~预热~~：**已按用户要求撤除**，`pyarx_qml.prewarm()` 函数已从 `pyarx_qml.py` 删除，面板一律 `show()` 直接打开。

## 六、面板打开的实际耗时（直接 show，不预热）

不预热、直接 `show()`（= 建侧栏 + `ensure` + `attach`）在 CAD 进程内的实测：

| 时间 | 构造本身 | setSource | SetParent | show() 总 | 第 1 帧 |
|---|---|---|---|---|---|
| 15:18:24 | 3590.6 ms | 520.9 ms | 48.8 ms | **4226.0 ms** | +4250.6 ms |
| 13:02:11 | 3538.3 ms | — | — | 3553.4 ms（只 ensure） | — |

也就是说第一次开面板约 **3.6~4.2 秒**，其中 3.5 秒是 Qt 扫 AutoCAD 目录，剩余的 attach（setSource + SetParent）在 0.5 秒上下。

历史记录（已撤除的预热方案）：`prewarm()` 3507.8 ms 之后 `show()` 只需 601.5 ms（14:49:01）。数字留着备查，但方案不采用。


## 七、复现命令

```powershell
# 探针模式文件：第一行 core / gui，可追加 qpa_dir= platform= plugin_root= qpa_log=1 debug_plugins=1
@("gui", "qpa_log=1", "debug_plugins=1") | Set-Content G:\PyArx\docs\_probe_mode.txt -Encoding utf8
Get-Process acad | Stop-Process -Force
powershell -NoProfile -ExecutionPolicy Bypass -File G:\PyArx\tests\cad\run_com.ps1 `
  -PyScript G:\PyArx\tests\qt_spike\pyarx_qml_probe_cad.py -Tag probe_dbgplug
# 结果看 G:\PyArx\docs\_pyarx_qml_probe_cad.log（run_com 会转后台，用 TaskOutput 取）
```

验证脚本：`G:\PyArx\tests\qt_spike\pyarx_qml_show_in_cad.py`，日志 `docs\_pyarx_qml_show_cad.log`，截图 `docs\_pyarx_qml_show_cad.png`。
