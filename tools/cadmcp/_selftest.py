"""cadmcp 自检：走 stdio 的完整 JSON-RPC 会话，验证四个工具真的能用。

用法：G:\\PyArx\\X64\\Release\\python.exe G:\\PyArx\\tools\\cadmcp\\_selftest.py [--no-cad]
"""
import json
import os
import subprocess
import sys

BASE = r"G:\PyArx\tools\cadmcp"
SERVER = os.path.join(BASE, "server.py")


def talk(msgs):
    proc = subprocess.Popen([sys.executable, SERVER], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            text=True, encoding="utf-8", bufsize=1)
    out = []
    for m in msgs:
        proc.stdin.write(json.dumps(m, ensure_ascii=False) + "\n")
        proc.stdin.flush()
        if m.get("id") is not None:
            line = proc.stdout.readline()
            out.append(json.loads(line))
    proc.stdin.close()
    proc.wait(timeout=10)
    err = proc.stderr.read()
    return out, err


def main():
    no_cad = "--no-cad" in sys.argv
    msgs = [
        {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {"protocolVersion": "2024-11-05"}},
        {"jsonrpc": "2.0", "method": "notifications/initialized"},
        {"jsonrpc": "2.0", "id": 2, "method": "tools/list"},
        {"jsonrpc": "2.0", "id": 3, "method": "tools/call",
         "params": {"name": "pyarx_projects", "arguments": {}}},
        {"jsonrpc": "2.0", "id": 4, "method": "tools/call",
         "params": {"name": "pyarx_projects", "arguments": {"query": "PyMfcXml"}}},
        {"jsonrpc": "2.0", "id": 5, "method": "tools/call",
         "params": {"name": "pyarx_api_search", "arguments": {"query": "PyDbTrans", "limit": 3}}},
    ]
    if not no_cad:
        msgs.append({"jsonrpc": "2.0", "id": 6, "method": "tools/call", "params": {
            "name": "pyarx_run", "arguments": {"timeout_sec": 60, "code":
                "db = P.Db.curDb()\n"
                "result = {'file': db.getFilename(),"
                " 'ents': len(db.objectIds('AcDbEntity')),"
                " 'texts': len(db.objectIds('AcDbText'))}\n"}}})
    replies, err = talk(msgs)

    r = {x.get("id"): x for x in replies}
    init = r[1]["result"]
    print("initialize:", init["serverInfo"], "| caps:", list(init["capabilities"]))
    print("tools:", [t["name"] for t in r[2]["result"]["tools"]])

    def payload(i):
        return json.loads(r[i]["result"]["content"][0]["text"])

    p_all = payload(3)
    print("pyarx_projects(full): ok=%s chars=%d sections=%d" % (
        p_all.get("ok"), len(p_all.get("text", "")), p_all.get("text", "").count("\n## ")))
    p_q = payload(4)
    print("pyarx_projects(query=PyMfcXml): ok=%s chars=%d first=%r" % (
        p_q.get("ok"), len(p_q.get("text", "")), p_q.get("text", "")[:40]))
    p_api = payload(5)
    print("pyarx_api_search(PyDbTrans): ok=%s count=%s" % (p_api.get("ok"), p_api.get("count")))
    if not no_cad:
        p_run = payload(6)
        print("pyarx_run:", json.dumps(p_run, ensure_ascii=False)[:300])
    if err.strip():
        print("stderr:", err.strip()[:400])


if __name__ == "__main__":
    main()
