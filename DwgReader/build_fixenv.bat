@echo off
rem 临时修复：vcvars64 未自动挂上 Windows SDK（WindowsSDKVersion 为空），
rem 手动补 INCLUDE/LIB 指向已存在的 10.0.26100.0 SDK，再调指定构建脚本。
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

set "SDK=C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0"
set "SDKLIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0"
set "INCLUDE=%INCLUDE%;%SDK%\ucrt;%SDK%\shared;%SDK%\um"
set "LIB=%LIB%;%SDKLIB%\ucrt\x64;%SDKLIB%\um\x64"

call %1
