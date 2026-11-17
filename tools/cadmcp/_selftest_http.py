import json
import sys
import urllib.request

URL = sys.argv[1] if len(sys.argv) > 1 else "http://127.0.0.1:8765/mcp"


def post(msg):
    req = urllib.request.Request(URL, data=json.dumps(msg).encode("utf-8"),
                                 headers={"Content-Type": "application/json",
                                          "Accept": "application/json, text/event-stream"},
                                 method="POST")
    with urllib.request.urlopen(req, timeout=120) as r:
        return json.loads(r.read().decode("utf-8")), dict(r.headers)


r, h = post({"jsonrpc": "2.0", "id": 1, "method": "initialize",
             "params": {"protocolVersion": "2024-11-05"}})
print("initialize:", r["result"]["serverInfo"], "| session:", h.get("Mcp-Session-Id"))

r, _ = post({"jsonrpc": "2.0", "id": 2, "method": "tools/list"})
print("tools:", [t["name"] for t in r["result"]["tools"]])

r, _ = post({"jsonrpc": "2.0", "id": 3, "method": "tools/call",
             "params": {"name": "pyarx_projects", "arguments": {"query": "cadmcp"}}})
p = json.loads(r["result"]["content"][0]["text"])
print("pyarx_projects(HTTP):", p.get("ok"), len(p.get("text", "")), "chars")

r, _ = post({"jsonrpc": "2.0", "id": 4, "method": "tools/call",
             "params": {"name": "pyarx_run", "arguments": {
                 "timeout_sec": 60,
                 "code": "db = P.Db.curDb()\nresult = {'file': db.getFilename(), 'ents': len(db.objectIds('AcDbEntity'))}\n"}}})
p = json.loads(r["result"]["content"][0]["text"])
print("pyarx_run(HTTP):", json.dumps(p, ensure_ascii=False)[:200])
