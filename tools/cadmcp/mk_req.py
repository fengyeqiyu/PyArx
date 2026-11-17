"""mk_req.py —— 把一段 PyArx 代码打包成 cadmcp 的 req.json。

用法：python mk_req.py <代码文件.py> [请求id]
"""
import json
import sys
import uuid

BASE = r"G:\PyArx\tools\cadmcp"
REQ = BASE + r"\req.json"


def main():
    src_path = sys.argv[1]
    rid = sys.argv[2] if len(sys.argv) > 2 else uuid.uuid4().hex[:8]
    with open(src_path, encoding="utf-8") as f:
        code = f.read()
    with open(REQ, "w", encoding="utf-8") as f:
        json.dump({"id": rid, "code": code}, f, ensure_ascii=False)
    print("req ok -> " + rid)


if __name__ == "__main__":
    main()
