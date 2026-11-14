"""xml_render / xml_layout 的 stdio 自测：一次性喂 JSON-RPC，收全部回声再判。

不要用逐行 readline —— server 的 stderr 管道不读时会把子进程卡死。
"""
import json, subprocess, sys

SRV = r"G:\PyArx\tools\cadmcp\server.py"
XML = r"G:\PyArx\examples\ui\power_plan_shell.xml"


def main():
    reqs = [
        {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {}},
        {"jsonrpc": "2.0", "method": "notifications/initialized"},
        {"jsonrpc": "2.0", "id": 2, "method": "tools/list"},
        {"jsonrpc": "2.0", "id": 3, "method": "tools/call",
         "params": {"name": "xml_render", "arguments": {"path": XML}}},
        {"jsonrpc": "2.0", "id": 4, "method": "tools/call",
         "params": {"name": "xml_layout", "arguments": {"path": XML, "max_lines": 20}}},
        {"jsonrpc": "2.0", "id": 5, "method": "tools/call",
         "params": {"name": "xml_render",
                    "arguments": {"path": r"G:\PyArx\does_not_exist.xml"}}},
    ]
    payload = ("\n".join(json.dumps(r, ensure_ascii=False) for r in reqs) + "\n")
    proc = subprocess.run([sys.executable, SRV], input=payload.encode("utf-8"),
                          capture_output=True, timeout=90)
    out, err = proc.stdout.decode("utf-8"), proc.stderr.decode("utf-8", "replace")
    replies = [json.loads(x) for x in out.splitlines() if x.strip()]

    tools = [t["name"] for t in replies[1]["result"]["tools"]]
    assert "xml_render" in tools and "xml_layout" in tools, tools
    print("tools:", tools)

    r3 = json.loads(replies[2]["result"]["content"][0]["text"])
    print("render:", r3.get("png"), r3.get("width"), "x", r3.get("height"),
          "%.2fs" % r3.get("elapsed_sec", 0), "ok=", r3.get("ok"))
    assert r3["ok"] and r3.get("width") == 1360 and r3.get("height") == 860

    r4 = json.loads(replies[3]["result"]["content"][0]["text"])
    print("layout:", r4.get("lines"), "行 truncated=", r4.get("truncated"))
    assert r4["ok"] and r4["lines"] > 20

    r5 = replies[4]["result"]
    print("错误路径:", r5["isError"], r5["content"][0]["text"][:80])
    assert r5["isError"] is True

    if err.strip():
        print("stderr:", err.strip()[:200])
    print("SELFTEST OK")


if __name__ == "__main__":
    main()
