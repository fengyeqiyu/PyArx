"""一次性迁移：xml -> xml（文件后缀、目录、工程名、标识符、字符串）。

只处理文本文件；.git / X64（编译产物）/ 二进制一律跳过。
"""
import io
import os
import sys

ROOT = r"G:\PyArx"
SKIP_DIRS = {".git", "X64", "obj", "bin", ".vs", "__pycache__", ".venv", "node_modules", "docs"}

TEXT_EXT = {
    ".h", ".hpp", ".hh", ".cpp", ".c", ".cc", ".cs", ".py", ".pyi", ".ps1", ".cmd", ".bat",
    ".xml", ".xml", ".sln", ".vcxproj", ".filters", ".md", ".txt", ".json", ".yml", ".yaml",
    ".qrc", ".rc", ".def", ".inl", ".iss", ".ini", ".tpl", ".ui", ".qml", ".html", ".css",
    ".js", ".cfg", ".toml", ".props", ".targets", ".natvis", ".msbuild", ".psm1",
}

MAP = (("Xml", "Xml"), ("xml", "xml"), ("XML", "XML"))


def map_name(name):
    for a, b in MAP:
        name = name.replace(a, b)
    return name


def is_text(path):
    ext = os.path.splitext(path)[1].lower()
    if ext in {".vcxproj.filters"}:
        return True
    return ext in TEXT_EXT


def read_text(path):
    raw = io.open(path, "rb").read()
    bom = b""
    if raw.startswith(b"\xef\xbb\xbf"):
        bom, raw = b"\xef\xbb\xbf", raw[3:]
    try:
        return raw.decode("utf-8"), bom
    except UnicodeDecodeError:
        return raw.decode("gbk"), bom


def write_text(path, text, bom):
    io.open(path, "wb").write(bom + text.encode("utf-8"))


def walk():
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS and "xml" not in d]
        # 目录改名交给 rename_pass（这里只跳进去，不改名）
        for f in filenames:
            yield os.path.join(dirpath, f)


def rename_pass():
    """先把带 xml 的目录 / 文件改名（从最深层开始）。"""
    renamed = 0
    for dirpath, dirnames, filenames in os.walk(ROOT, topdown=False):
        parts = os.path.relpath(dirpath, ROOT).split(os.sep)
        if any(p in SKIP_DIRS for p in parts):
            continue
        for d in dirnames:
            if "xml" in d.lower():
                src = os.path.join(dirpath, d)
                dst = os.path.join(dirpath, map_name(d))
                if os.path.exists(dst):
                    print("SKIP(dup)", dst)
                    continue
                os.rename(src, dst)
                renamed += 1
    for dirpath, dirnames, filenames in os.walk(ROOT):
        parts = os.path.relpath(dirpath, ROOT).split(os.sep)
        if any(p in SKIP_DIRS for p in parts):
            continue
        for f in filenames:
            if "xml" in f.lower():
                src = os.path.join(dirpath, f)
                dst = os.path.join(dirpath, map_name(f))
                if os.path.exists(dst):
                    print("SKIP(dup)", dst)
                    continue
                os.rename(src, dst)
                renamed += 1
    return renamed


def content_pass():
    changed = 0
    for path in walk():
        if not is_text(path):
            continue
        try:
            text, bom = read_text(path)
        except Exception as e:
            print("READFAIL", path, e)
            continue
        new = text
        for a, b in MAP:
            new = new.replace(a, b)
        if new != text:
            write_text(path, new, bom)
            changed += 1
    return changed


if __name__ == "__main__":
    n = rename_pass()
    c = content_pass()
    print("renamed paths:", n, "rewritten files:", c)
