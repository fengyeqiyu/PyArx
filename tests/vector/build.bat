@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
cd /d G:\PyArx\tests\vector
if not exist bin mkdir bin
cl /nologo /utf-8 /std:c++17 /O2 /EHsc /I G:\PyArx\MfcXmlCore dwg_render.cpp /Fe:bin\dwg_render.exe /Fo:bin\
echo EXIT=%ERRORLEVEL%
