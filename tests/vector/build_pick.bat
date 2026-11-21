@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
set SDK=C:\Program Files (x86)\Windows Kits\10
set SDKV=10.0.28000.0
set INCLUDE=%INCLUDE%;%SDK%\Include\%SDKV%\ucrt;%SDK%\Include\%SDKV%\um;%SDK%\Include\%SDKV%\shared
set LIB=%LIB%;%SDK%\Lib\%SDKV%\ucrt\x64;%SDK%\Lib\%SDKV%\um\x64
cd /d G:\PyArx\tests\vector
if not exist bin mkdir bin
cl /nologo /utf-8 /std:c++17 /O2 /EHsc /I G:\PyArx\MfcXmlCore %1 /Fe:bin\%~n1.exe /Fo:bin\ >pick_build.log 2>&1
echo EXIT=%ERRORLEVEL%
type pick_build.log
