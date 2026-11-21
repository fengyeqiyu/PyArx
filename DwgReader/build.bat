@echo off
rem Build reader.dll (with import lib). Pure self-developed decoder, no libredwg.
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

set "SRC=G:\PyArx\DwgReader"
set "INC=/I %SRC%\include"
set "BUILD=%SRC%\build"
if not exist "%BUILD%" mkdir "%BUILD%"

cl /nologo /O2 /MD /EHsc /W3 /utf-8 /std:c++17 /wd4251 /LD /DDWREADER_BUILD_DLL %INC% ^
  "%SRC%\src\rx\class.cpp" ^
  "%SRC%\src\io\codec\decompress.cpp" ^
  "%SRC%\src\io\codec\decompress1021.cpp" ^
  "%SRC%\src\io\codec\bitstream.cpp" ^
  "%SRC%\src\io\filer\dwg_out_filer.cpp" ^
  "%SRC%\src\io\container\container.cpp" ^
  "%SRC%\src\io\container\classic_container.cpp" ^
  "%SRC%\src\io\container\classic_writer.cpp" ^
  "%SRC%\src\io\container\container1021.cpp" ^
  "%SRC%\src\db\database.cpp" ^
  "%SRC%\src\db\index\handle_map.cpp" ^
  "%SRC%\src\db\index\object_index.cpp" ^
  "%SRC%\src\db\entity\registry.cpp" ^
  "%SRC%\src\db\entity\entities.cpp" ^
  "%SRC%\src\db\entity\decode.cpp" ^
  "%SRC%\src\db\entity\encode.cpp" ^
  /Fe"%BUILD%\reader.dll" /Fo"%BUILD%\\"
