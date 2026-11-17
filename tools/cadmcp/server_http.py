"""cadmcp HTTP 入口 —— 把 server.py 的工具用 MCP streamable HTTP 暴露出来。

豆包/其它客户端的 MCP 导入通道只接受 streamable HTTP（transport_type=2 + url），
stdio 版本没法被导入，所以这里起一个本机 HTTP 端点，复用 server.py 的 dispatch
（工具清单直接取 core.TOOLS，server.py 加工具这里自动跟着走，但要重启本进程才生效）。

启动：python server_http.py [--port 8765] [--host 127.0.0.1]
端点：POST /mcp   JSON-RPC（单条或批量）；GET /health 健康检查
"""

import argparse
import json
import os
import sys
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import server as core  # noqa: E402

SESSION_HEADER = "Mcp-Session-Id"


def rpc(msg):
    mid = msg.get("id")
    method = msg.get("method", "")
    params = msg.get("params") or {}

    if method == "initialize":
        return {"jsonrpc": "2.0", "id": mid, "result": {
            "protocolVersion": params.get("protocolVersion", "2024-11-05"),
            "capabilities": {"tools": {}},
            "serverInfo": {"name": "cadmcp", "version": "0.1.0"},
        }}
    if method in ("notifications/initialized", "initialized"):
        return None
    if method == "ping":
        return {"jsonrpc": "2.0", "id": mid, "result": {}}
    if method == "tools/list":
        return {"jsonrpc": "2.0", "id": mid, "result": {"tools": core.TOOLS}}
    if method == "tools/call":
        name = params.get("name")
        args = params.get("arguments") or {}
        try:
            out = core.dispatch(name, args)
        except Exception as e:
            out = {"ok": False, "error": repr(e)}
        return {"jsonrpc": "2.0", "id": mid, "result": {
            "content": [{"type": "text", "text": json.dumps(out, ensure_ascii=False)}],
            "isError": not out.get("ok", False),
        }}
    return {"jsonrpc": "2.0", "id": mid,
            "error": {"code": -32601, "message": "method not found: " + method}}


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    server_version = "cadmcp-http/0.1"

    def log_message(self, fmt, *args):
        sys.stderr.write("[cadmcp-http] " + (fmt % args) + "\n")

    def _send_json(self, code, obj, sid=None):
        body = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        if sid:
            self.send_header(SESSION_HEADER, sid)
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path.split("?")[0] in ("/health", "/"):
            self._send_json(200, {"ok": True, "server": "cadmcp", "tools": [t["name"] for t in core.TOOLS]})
            return
        # 不支持服务端主动推送：按 streamable HTTP 规范回 405
        self._send_json(405, {"error": "SSE stream not supported; use POST /mcp"})

    def do_DELETE(self):
        self._send_json(200, {"ok": True})

    def do_POST(self):
        if self.path.split("?")[0] != "/mcp":
            self._send_json(404, {"error": "not found: " + self.path})
            return
        length = int(self.headers.get("Content-Length") or 0)
        raw = self.rfile.read(length) if length else b""
        try:
            payload = json.loads(raw.decode("utf-8"))
        except Exception as e:
            self._send_json(400, {"jsonrpc": "2.0", "id": None,
                                  "error": {"code": -32700, "message": "parse error: %r" % (e,)}})
            return

        sid = self.headers.get(SESSION_HEADER) or uuid.uuid4().hex

        if isinstance(payload, list):
            replies = [r for r in (rpc(m) for m in payload) if r is not None]
            if not replies:
                self.send_response(202)
                self.send_header("Content-Length", "0")
                self.send_header(SESSION_HEADER, sid)
                self.end_headers()
                return
            self._send_json(200, replies, sid)
            return

        reply = rpc(payload)
        if reply is None:
            self.send_response(202)
            self.send_header("Content-Length", "0")
            self.send_header(SESSION_HEADER, sid)
            self.end_headers()
            return
        self._send_json(200, reply, sid)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8765)
    a = ap.parse_args()
    srv = ThreadingHTTPServer((a.host, a.port), Handler)
    srv.daemon_threads = True
    sys.stderr.write("[cadmcp-http] listening on http://%s:%d/mcp\n" % (a.host, a.port))
    sys.stderr.flush()
    srv.serve_forever()


if __name__ == "__main__":
    main()
