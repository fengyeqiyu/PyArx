@echo off
rem Bypass single-file compile check. Usage: check_one.bat PyDbViewport.cpp [-ShowWarnings]
rem Multiple source files may be passed at once. Output obj goes to %TEMP%\pyarx_check_obj.
setlocal
set PS=powershell
where pwsh >nul 2>nul && set PS=pwsh
"%PS%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0check_one.ps1" %*
exit /b %ERRORLEVEL%
