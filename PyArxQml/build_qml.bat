@echo off
setlocal
set QT=G:\Qt\6.2.6\msvc2019_64
set QTINC=%QT%\include
set CADLIBS=G:\PyArx\third_party\qt626
set PY=D:\Program Files\Python314
set SRC=G:\PyArx\PyArxQml
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%SRC%"

rem Q_OBJECT 类必须先 moc
"%QT%\bin\moc.exe" QmlBackend.h -o moc_QmlBackend.cpp
if errorlevel 1 goto :fail

rem 链接库来自 CAD 自己的 Qt 6.2.6 DLL 导出表（third_party\qt626），
rem 头文件用本机源码编译的 Qt 6.2.6（G:\Qt\6.2.6\msvc2019_64）。
rem 已改用 QGuiApplication：不再需要 QtWidgets。
cl /nologo /LD /EHsc /std:c++17 /Zc:__cplusplus /utf-8 /MD /O2 /D_CRT_SECURE_NO_WARNINGS /DQT_NO_KEYWORDS PyArxQml.cpp QmlBackend.cpp PyArxQmlLog.cpp PyArxQmlModule.cpp moc_QmlBackend.cpp /I "%PY%\Include" /I "%SRC%" /I "%QTINC%" /I "%QTINC%\QtCore" /I "%QTINC%\QtGui" /I "%QTINC%\QtQml" /I "%QTINC%\QtQuick" /Fe:PyArxQml.pyd /link /LIBPATH:"%CADLIBS%" /LIBPATH:"%PY%\libs" python314.lib Qt6Core.lib Qt6Gui.lib Qt6Qml.lib Qt6Quick.lib user32.lib gdi32.lib
echo BUILD_EXIT=%errorlevel%

if exist PyArxQml.pyd (
  dir /b PyArxQml.pyd
  copy /y PyArxQml.pyd "G:\PyArx\X64\Release\PyArxQml.pyd" >nul
  echo COPIED_TO_RELEASE
)

:fail
endlocal
