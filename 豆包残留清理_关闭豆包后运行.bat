@echo off
setlocal enabledelayedexpansion
title Doubao Cache Cleanup
set "D=%LOCALAPPDATA%\Doubao\User Data\Default"

tasklist /fi "imagename eq Doubao.exe" 2>nul | find /i "Doubao.exe" >nul
if not errorlevel 1 (
    echo [STOP] Doubao is still running. Quit Doubao completely first, then run this file again.
    pause
    exit /b 1
)

echo Cleaning locked cache/storage folders...
for %%d in ("Cache" "Code Cache" "Safe Browsing Network" "Site Characteristics Database" "Shared Dictionary" "Platform Notifications" "Sync Data" "Session Storage" "WebStorage" "IndexedDB" "Local Storage" "shared_proto_db" "Service Worker" "GCM Store" "Extension State" "saman_shell_db_storage" "saman_shell_persistent_storage_v1" "trust_layer" "Network" "Sessions") do (
    if exist "%D%\%%~d" (
        rd /s /q "%D%\%%~d"
        if exist "%D%\%%~d" (echo   [FAILED] %%~d) else (echo   [OK] %%~d)
    )
)

echo Cleaning agent workspace (history / memory / sessions)...
rd /s /q "%D%\.doubao\agent_mode\workspace\conversation"
rd /s /q "%D%\.doubao\agent_mode\workspace\.sessions"
rd /s /q "%D%\agent_infra\to"
rd /s /q "%D%\DoubaoStorage\Aida\doubao-office"

echo.
echo Done. Remaining size of Doubao user data:
powershell -NoProfile -Command "$s=(Get-ChildItem -LiteralPath $env:LOCALAPPDATA'\Doubao\User Data\Default' -Recurse -Force -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum; '{0:N1} MB' -f ($s/1MB)"
echo.
echo Note: deleting Network cookies logs you out of Doubao web sessions; sign in again if asked.
pause
