# hotbuild —— PyArxCore 热编译

改完 C++ 源码后，**不重启 acad.exe** 让新代码生效。

## 用

```powershell
# 构建 + 只产出新 pyd（不改动 CAD 里的模块）
powershell -File G:\PyArx\tools\hotbuild\hotbuild.ps1 -Tag h1

# 让 CAD 真正切到新版本（可在 CAD 内跑，也可经 cadmcp）
# 方式 A：cadmcp 的 pyarx_hotbuild 工具（推荐，一步到位）
# 方式 B：手工
#   (command "PYRUNFILE" "G:/PyArx/tools/hotbuild/reload.py")
```

产物：`G:\PyArx\X64\Release\PyArxCoreH<tag>.pyd`，同时 `module.txt` 记下最新模块名。

## 机制（四件事缺一不可）

1. **模块名宏化**：`PyArxBinder.cpp` 用 `PYBIND11_MODULE(PYARX_HOT_MODULE, m)`，
   名字来自 `HotModName.h`（只被该 TU 包含，所以换名只重编一个文件 + 链接）。
   pybind 要求 `PyInit_<stem>` 与文件名一致，因此必须真改名，不能只改文件名。
2. **internals 隔离**：热编译用恒定 `PYBIND11_INTERNALS_KIND = "_dev"`（`HotModuleName.h`，
   被 stdafx 强制包含）。恒定才不会每次全量重编；同名类型重复注册由入口处
   `registered_types_cpp/py.clear()` 解决（仅当模块名 ≠ PyArxCore 时执行）。
3. **子模块别名**：各 binder 内部按 `"PyArxCore.Ge"` 之类互相 import，
   入口用 `sub()` 把每个子模块同时登记到 `sys.modules["PyArxCore.X"]`，
   否则热模块会报 `PyArxCore is not a package`。
4. **增量构建**：`hotbuild.ps1` 只在内容变化时写恒定头文件；
   重写同名头哪怕内容不变也会刷新 mtime，触发 500+ 文件全量重编（约 9 分钟 vs 13 秒）。

## 时间

| 场景 | 耗时 |
|---|---|
| 改 .cpp 后热编译（增量） | ≈13 秒 |
| 从常规构建首次切到 hot 模式 | 全量一次 ≈9 分钟 |
| 改公共头文件（如 stdafx.h） | 全量 |

## 已知边界

- 新旧两代 pyd 是两套 C++ 类型：**跨代对象不能互相传参**（例如把旧模块的 `PyDbTrans`
  传给新模块函数）。热替换后请整段脚本重跑。
- 旧模块的 Python 类型对象仍在内存里（DLL 不能被 CPython 卸载），所以每热编译一次
  进程里多一份 DLL；长时间开发建议每隔若干次重启 CAD 清一次。
- 切回常规（发布）构建：把 `PyArxCore\HotModuleName.h` 改回 `PYARX_HOT_INTERNALS` 为空、
  `HotModName.h` 保持 `PyArxCore`，再全量构建一次，输出真正的 `PyArxCore.pyd`。

## 为什么不用「释放 DLL 再加载同名模块」

实测结论（CPython 3.15t）：

- 从 `sys.modules` 摘掉后重新 `import` 同一路径，CPython 的 extension 模块缓存会直接返回
  原来那个模块对象，**不会重新执行 init**，等于没换代码。
- pybind11 的类型注册表挂在解释器状态里，不是 Python 层可清理的字典，
  同名同文件再初始化时仍会撞 `type ... is already registered`。
- 因此走「新模块名 + 清注册表」，等效于热替换，且不需要动被映射的旧文件。

（`reload_same.py` 保留作研究用，未接入主流程。）
