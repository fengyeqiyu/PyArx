@echo off
setlocal
set QT=G:\Qt\6.2.4\msvc2019_64
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d G:\PyArx\tests\qt_spike
cl /nologo /EHsc /std:c++17 /Zc:__cplusplus /utf-8 /MD /O2 quick_grab.cpp /I "%QT%\include" /I "%QT%\include\QtCore" /I "%QT%\include\QtGui" /I "%QT%\include\QtWidgets" /I "%QT%\include\QtQml" /I "%QT%\include\QtQuick" /Fe:qt_quick_grab.exe /link /LIBPATH:"%QT%\lib" Qt6Core.lib Qt6Gui.lib Qt6Widgets.lib Qt6Qml.lib Qt6Quick.lib user32.lib gdi32.lib
echo BUILD_EXIT=%errorlevel%
endlocal