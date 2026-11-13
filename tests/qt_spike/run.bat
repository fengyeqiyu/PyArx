@echo off
setlocal
set QT=G:\Qt\6.2.4\msvc2019_64
set PATH=%QT%\bin;%PATH%
set QTWEBENGINEPROCESS_PATH=%QT%\bin\QtWebEngineProcess.exe
set QTWEBENGINE_RESOURCES_PATH=%QT%\resources
set QTWEBENGINE_LOCALES_PATH=%QT%\translations\qtwebengine_locales
cd /d G:\PyArx\tests\qt_spike
qt_spike.exe
echo RUN_EXIT=%errorlevel%
endlocal