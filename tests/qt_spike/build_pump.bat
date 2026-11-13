@echo off
setlocal
set QT=G:\Qt\6.2.4\msvc2019_64
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d G:\PyArx\tests\qt_spike
cl /nologo /EHsc /std:c++17 /Zc:__cplusplus /utf-8 /MD /O2 /D_CRT_SECURE_NO_WARNINGS pump_test.cpp /I "%QT%\include" /I "%QT%\include\QtCore" /I "%QT%\include\QtGui" /I "%QT%\include\QtWidgets" /I "%QT%\include\QtWebEngineWidgets" /I "%QT%\include\QtWebEngineCore" /I "%QT%\include\QtNetwork" /Fe:qt_pump.exe /link /LIBPATH:"%QT%\lib" Qt6Core.lib Qt6Gui.lib Qt6Widgets.lib Qt6WebEngineWidgets.lib Qt6WebEngineCore.lib Qt6Network.lib user32.lib gdi32.lib
echo BUILD_EXIT=%errorlevel%
endlocal