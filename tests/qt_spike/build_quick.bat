@echo off
setlocal
set QT=G:\Qt\6.2.4\msvc2019_64
set QTINC=%QT%\include
set CADLIBS=G:\PyArx\tests\qt_spike\cadlibs
set PY=D:\Program Files\Python314
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d G:\PyArx\tests\qt_spike

rem Q_OBJECT 类必须先跑 moc
"%QT%\bin\moc.exe" QtBackend.h -o moc_QtBackend.cpp
if errorlevel 1 goto :fail

rem 链接用 cadlibs（从 CAD 自己的 Qt 6.2.6 DLL 导出的 import lib），
rem 保证符号面与运行时完全一致；头文件仍是 6.2.4（6.2.x 补丁级 API 一致）。
cl /nologo /LD /EHsc /std:c++17 /Zc:__cplusplus /utf-8 /MD /O2 /D_CRT_SECURE_NO_WARNINGS /DQT_NO_KEYWORDS QtQuickSpike.cpp QtBackend.cpp moc_QtBackend.cpp /I "%PY%\Include" /I "%QTINC%" /I "%QTINC%\QtCore" /I "%QTINC%\QtGui" /I "%QTINC%\QtWidgets" /I "%QTINC%\QtQml" /I "%QTINC%\QtQuick" /Fe:QtQuickSpike.pyd /link /LIBPATH:"%CADLIBS%" /LIBPATH:"%PY%\libs" python314.lib Qt6Core.lib Qt6Gui.lib Qt6Widgets.lib Qt6Qml.lib Qt6Quick.lib user32.lib gdi32.lib
echo BUILD_EXIT=%errorlevel%
if exist QtQuickSpike.pyd dir /b QtQuickSpike.pyd

:fail
endlocal
