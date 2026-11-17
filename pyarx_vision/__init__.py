# -*- utf-8 -*-
"""pyarx_vision —— 矢量图纸识图能力

对外只暴露三件事：
    load(path, kind=None) -> EntitySet     读一种图元导出文件并归一
    recognize(es, config=None) -> Result    识别（原生几何 + 语义 + 自检）
    Result.to_design() -> dict              转成电力设计数据模型

用法：
    import pyarx_vision as V
    es = V.load(r"C:/temp/sys.json", "sys")
    res = V.recognize(es)
    if res.ok:
        design = res.to_design()
    else:
        for p in res.problems: print(p)
"""
import json

from .schema import (empty, add_line, add_pline, add_text, add_circle,
                     bbox, validate)
from .adapters import load, LOADERS
from .recognize import recognize, Result, DEFAULT_CONFIG

__all__ = ["empty", "add_line", "add_pline", "add_text", "add_circle", "bbox",
           "validate", "load", "LOADERS", "recognize", "Result",
           "DEFAULT_CONFIG", "dump_design", "load_design"]
__version__ = "1.0.0"


def dump_design(design, path):
    json.dump(design, open(path, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    return path


def load_design(path):
    return json.load(open(path, encoding="utf-8"))
