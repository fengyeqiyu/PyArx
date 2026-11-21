@echo off
rem Build tools\verify_write.exe against reader.lib (needs reader.dll already built).
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

set "SRC=G:\PyArx\DwgReader"
set "BUILD=%SRC%\build"

cl /nologo /O2 /MD /EHsc /W3 /utf-8 /std:c++17 /I "%SRC%\include" ^
  "%SRC%\tools\verify_write.cpp" ^
  /Fe"%BUILD%\verify_write.exe" /Fo"%BUILD%\\" "%BUILD%\reader.lib"
