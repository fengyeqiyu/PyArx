' cadmcp HTTP 服务隐藏启动器（不闪窗）
' 用法：wscript "G:\PyArx\tools\cadmcp\start_http.vbs"
Dim sh, py, srv, cmd
Set sh = CreateObject("WScript.Shell")
py  = "G:\PyArx\X64\Release\python.exe"
srv = "G:\PyArx\tools\cadmcp\server_http.py"

' 已在监听就不重复启动
Dim probe
probe = "cmd /c netstat -ano | findstr /c:""127.0.0.1:8765"" | findstr /c:""LISTENING"" >nul 2>&1"
If sh.Run(probe, 0, True) = 0 Then WScript.Quit 0

cmd = """" & py & """" & " """ & srv & """ --port 8765"
sh.Run cmd, 0, False
