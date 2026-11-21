# DwgReader

独立 C++ DWG 解析**动态链接库**，纯自研解码，**不依赖 libredwg**。
将离线 Python 已验证的自研链路（容器解压 + 位流 + 实体类）移植到 C++；
供其他 C++ 项目链接调用，后续再浅封装为 pybind11 `.pyd`。

版本支持：AC1018 (R2004)、AC1024 (R2010)、AC1027 (R2013)、AC1032 (R2018) 页容器链全部打通并各自解出实体；
AC1021 (R2007/R2008/R2009) 走独立容器链，与 Python 基准字节级对齐。
AC1012/1014/1015（R13/R14/R2000）经典布局：文件头 + 节定位器 + Handles 页句柄表已打通，
R14 对象头与类型码已正确，R2000 对象头 handle 仍错位（已知缺陷）。
逐版本口径见 `docs/dwg-version-decode-matrix.md`；写链逆向与整合方案见 `docs/arx-dwg-write-pipeline.md`；
多版本真样本由无头 AutoCAD 生成，方法见 `docs/headless-cad-version-export.md`。

## 框架分层

以 ObjectARX SDK 为蓝本、Rust 开源 CAD 库为参考，按解码依赖单向分层；每层对应未来 pyd 的一个子模块。核心层不链 ARX 运行时、不存 `PyObject*`、不引入 `std::variant`。

| 框架 | 目录 | ARX 蓝本 | 职责 | 状态 |
|---|---|---|---|---|
| **rx** | `rx/` | AcRx | `RxObject` 基类、`RxRegistry` 类型注册表 | 骨架落地 |
| **ge** | `ge/` | AcGe | Point2d/3d、Vector2d/3d、Matrix3d、Tol、Box3d、Curve 预留 | 值类型落地 |
| **db** | `db/` | AcDb | Entity/DbObject、符号表/字典、句柄/类型索引、Database 门面 | 归位 + 骨架 |
| **io** | `io/` | DwgFiler | codec 位流/解压、container 容器、filer 读写预留 | 读侧落地 |
| **gi** | `gi/` | AcGi | Drawable、GiWorldGeometry、GiViewport 预留 | 骨架 |
| **ed** | `ed/` | AcEd | Editor 选择集/拾取/Jig/Prompt 预留 | 骨架 |
| **ap** | `ap/` | AcAp | Application / Document 预留 | 骨架 |
| **bind** | `bind/` | serde 边界 | pybind11 浅封装，不进核心 | 待做（#17） |
| value | `value/` | — | 基础枚举/错误/版本（TypeCode、ErrorStatus、DwgVersion） | 已落地 |

依赖单向：`rx/ge`（无依赖）→ `db` → `io/gi` → `ed` → `ap`；`bind` 最外。禁止反向依赖，db 以下层不出现 Python/ARX 类型。

## 目录

```
include/dwg_reader/
  export.h            DWREADER_API 导入/导出宏
  value/              TypeCode / ErrorStatus / DwgVersion（Vec3 别名 ge::Point3d）
  rx/                 RxObject 基类、RxRegistry 注册表
  ge/                 Point/Vector/Matrix/Tol/Box3d/Curve 几何值类型
  db/
    database.h        Database 门面（容器/索引/实体表聚合）
    entity/           Entity 基类、注册表、各子类
    object/           DbObject 非图形对象基类
    table/            SymbolTable / Record 预留
    dictionary/       Dictionary 预留
    index/            HandleMap / ObjectIndex
  io/
    codec/            BitStream 位链、LZ77/RS 解压
    container/        Container / Container1021 容器与节
    filer/            DwgFiler 读写抽象、DwgWriter 写侧预留
  gi/                 Drawable / GiWorldGeometry / GiViewport
  ed/                 Editor 交互预留
  ap/                 Application / Document
  bind/               pybind11 边界（不进核心）
src/                   对应实现；src/bind/ 为 pyd 绑定层
tools/                 临时回归校验程序（verify_open），不进 DLL
build.bat              编译 reader.dll + 导入库 reader.lib
```

## 编译

```
build.bat
```

产物：`build/reader.dll`、`build/reader.lib`（导入库）、`build/reader.exp`。
仅依赖 VC 动态运行库（`/MD`），无第三方静态库。

## 被其他项目调用

- 链接 `reader.lib`，运行期加载 `reader.dll`；头文件根目录取 `include/`，引用路径 `dwg_reader/...`。
- 调用方需与本库同一 CRT：`/MD`（Release 动态运行库）。
- 编 DLL 时内部定义 `DWREADER_BUILD_DLL`；外部使用方默认走 dllimport。

```cpp
#include <dwg_reader/db/database.h>

dwr::Database db;
if (db.open("xxx.dwg")) {
    const auto& buf = db.objectsBuffer();  // 解压后的对象流
}
```

## 进度

- 已落地：容器层（头 LCG 解密、页地图/节地图定位、数据页掩码、节拼接解压）、
  LZ77 解压、位流原语。AcDb:AcDbObjects 与 AcDb:Handles 两节缓冲与 Python 基线
  SHA256 字节级一致（22,004,736B / 653,312B）。
- Handles 差分目录已完成：按页归零的 UMC 句柄增量 + MC 地址增量重建 handle→address，
  AC1021 业务图上 224912 条与 ARX 基准 handle/addr/type 全零差异（详见
  `docs/arx-dwg-read-pipeline.md` 验证段）。
- 待做：经典容器的对象头对齐（R2000 handle 错位）、R2000 句柄分页布局依赖（重建后 AutoCAD 报 53）、
  写侧实体 `writeFields` 与 R2004+ 页容器 writer（R14 容器重建已被 AutoCAD 接受，R2000 可逐字节复现）、
  R2007 容器以外的各代真实 AutoCAD 样本回归、字符串流（R2007+）用 TEXT/MTEXT 校验；
  补全各类型几何 getter；pybind11 浅封装输出 `.pyd`；再接 MfcXml 画布。
- R2007 (AC1021)：独立解码链（头 0x3d8→decode_rs→decompress_r2007、独立节/页地图、
  自有解压算法），参照 `docs/libredwg_src/src/decode_r2007.c` 平行移植，基准为业务 2007 图纸。
