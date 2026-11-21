@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
set "SRC=G:\PyArx\DwgReader"
cl /nologo /O2 /MD /EHsc /W3 /utf-8 /std:c++17 /wd4251 /I "%SRC%\include" ^
  "%SRC%\tools\verify_open.cpp" ^
  /Fe"%SRC%\build\verify.exe" /Fo"%SRC%\build\\" ^
  /link "%SRC%\build\reader.lib"
