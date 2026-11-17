"""从 PyRx 桩生成「PyArx 尚未注册」的枚举类 C++ 绑定。

输入：G:\\PyRx-main\\pyrx\\Py<Mod>.pyi  +  G:\\PyArx\\log\\enum_dump.json（运行时真实枚举快照）
输出：PyArxCore\\PyArxExtraEnums.h / .cpp
口径：只补 PyRx 有、PyArx 没有的枚举类型；值取 pyi 注释里的真实数值；名字与成员名原样照抄。
"""
import io
import json
import os
import re

PYRX = r"G:\PyRx-main\pyrx"
DUMP = r"G:\PyArx\log\enum_dump.json"
OUT_H = r"G:\PyArx\PyArxCore\PyArxExtraEnums.h"
OUT_CPP = r"G:\PyArx\PyArxCore\PyArxExtraEnums.cpp"

MODULES = ["Db", "Ge", "Ap", "Gi", "Ed", "Rx"]  # Pl/Sm/Gs 待定范围，暂不生成
BRX = {"Br", "Brx", "BrxBim", "BrxCv", "Ax"}

# 这些名字在 PyRx 里存在，但 PyArx 已有等价类型挂在别的名字/模块上 → 用别名，不新建类型。
# 必须在这里排除，否则先设了 module 属性、后面 py::enum_ 注册同名类型会抛
# "generic_type: cannot initialize type ...: an object with that name is already defined"
ALIASES = {"Db": {"ColorMethod", "TransparencyMethod"}}

CLS = re.compile(r"^class (\w+)\(_BoostPythonEnum\):")
MEM = re.compile(r"^\s+(\w+):\s*[^#]*?(?:#\s*(-?\d+))?\s*$")


def parse_pyi(path):
    enums = {}
    cur = None
    for line in io.open(path, encoding="utf-8", errors="ignore"):
        line = line.rstrip("\r\n")
        m = CLS.match(line)
        if m:
            cur = m.group(1)
            enums[cur] = {}
            continue
        if cur is None or not line.strip():
            continue
        if not line.startswith((" ", "\t")):
            cur = None
            continue
        mm = MEM.match(line)
        if mm and mm.group(2):
            enums[cur].setdefault(mm.group(1), int(mm.group(2)))
    return enums


def c_str(s):
    return '"%s"' % s


def main():
    # 基线 = 手工绑定的枚举清单（固定文件，避免拿运行时快照当基线导致自我递归跳过）
    base_path = r"G:\PyArx\docs_notes\api_parity\enum_baseline.txt"
    have = {}
    have_anywhere = {}
    for line in io.open(base_path, encoding="utf-8"):
        line = line.strip()
        if not line:
            continue
        mod, name = line.split(",")
        have.setdefault(mod, set()).add(name)
        have_anywhere.setdefault(name, mod)

    bodies = []
    tag_id = 0
    stats = {}
    for mod in MODULES:
        path = os.path.join(PYRX, "Py%s.pyi" % mod)
        want = parse_pyi(path)
        blocks = []
        for name, members in sorted(want.items()):
            if name in have.get(mod, set()):
                continue
            if name in have_anywhere:
                continue  # 别名可补，跳过
            if name in ALIASES.get(mod, set()):
                continue  # 由函数开头的别名赋值处理
            if not members:
                continue
            tag_id += 1
            tag = "Tag<%d>" % tag_id
            lines = ['\tpy::enum_<%s::type>(m, %s, %s)' % (tag, c_str(name),
                                                          c_str("PyRx 兼容枚举（值取自 PyRx 桩）"))]
            for i, (k, v) in enumerate(sorted(members.items())):
                prefix = "\t\t" if i == 0 else "\t\t"
                sep = "" if i == len(members) - 1 else ""
                lines.append('%s.value(%s, static_cast<%s::type>(%d))%s'
                             % (prefix, c_str(k), tag, v, sep))
            blocks.append(";\n".join(lines) if False else "\n".join(lines) + ";\n")
            stats[mod] = stats.get(mod, 0) + 1
        if blocks:
            bodies.append(("\t{\n\tauto m = py::reinterpret_borrow<py::module_>(root.attr(\"%s\"));\n"
                           % mod) + "\n".join(blocks) + "\t}\n")

    h = """#pragma once
// 生成物：tools/gen_extra_enums.py 产出，请勿手改；改法 = 改生成器后重跑。
// 内容：PyRx 有、PyArx 原本没有的枚举类（值取自 PyRx 桩里的真实数值）。
#include <pybind11/pybind11.h>

void bind_PyArxExtraEnums(pybind11::module_& root);
"""
    cpp = """#include "stdafx.h"
#include "PyArxExtraEnums.h"

namespace py = pybind11;

namespace {
// 每个枚举一个独立 tag 类型：pybind11 的 py::enum_ 需要真实的 C++ enum 类型做标识，
// 这些枚举只是「PyRx 命名常量表」，没有对应的 PyArx 原生枚举，故用模板 tag 造类型。
template <int N> struct Tag { enum type : int { }; };
}

void bind_PyArxExtraEnums(py::module_& root)
{
    // 别名：PyRx 在 Db 下也放这两个枚举名，PyArx 侧类型已存在于 Gi
    {
        auto db = py::reinterpret_borrow<py::module_>(root.attr("Db"));
        auto gi = py::reinterpret_borrow<py::module_>(root.attr("Gi"));
        db.attr("ColorMethod") = gi.attr("AcCmColorMethod");
        db.attr("TransparencyMethod") = gi.attr("AcCmTransparencyMethod");
    }

%s}
""" % ("\n".join(bodies))

    io.open(OUT_H, "w", encoding="utf-8", newline="").write(h)
    io.open(OUT_CPP, "w", encoding="utf-8", newline="").write(cpp)
    print("生成枚举：%s，合计 %d 个" % (stats, sum(stats.values())))


if __name__ == "__main__":
    main()
