@echo off
rem Build tools\verify_entity_write.exe directly from sources (no DLL exports needed).
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
set "SRC=G:\PyArx\DwgReader"
set "BUILD=%SRC%\build"
cl /nologo /O2 /MD /EHsc /W3 /utf-8 /std:c++17 /DDWREADER_BUILD_DLL /I "%SRC%\include" ^
  "%SRC%\tools\verify_entity_write.cpp" ^
  "%SRC%\src\io\codec\bitstream.cpp" ^
  "%SRC%\src\db\entity\decode.cpp" ^
  "%SRC%\src\db\entity\encode.cpp" ^
  "%SRC%\src\db\entity\entities.cpp" ^
  "%SRC%\src\db\entity\registry.cpp" ^
  "%SRC%\src\io\container\classic_container.cpp" ^
  /Fe"%BUILD%\verify_entity_write.exe" /Fo"%BUILD%\\"

