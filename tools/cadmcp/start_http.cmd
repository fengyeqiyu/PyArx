@echo off
rem cadmcp HTTP 入口（豆包/其它客户端的 streamable HTTP MCP 用）
start "" /min powershell -NoProfile -WindowStyle Hidden -Command "& 'G:\PyArx\X64\Release\python.exe' 'G:\PyArx\tools\cadmcp\server_http.py' --port 8765"
