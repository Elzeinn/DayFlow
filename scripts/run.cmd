@echo off
if "%QTDIR%"=="" set "QTDIR=D:\Qt\6.8.3\msvc2022_64"
set "PATH=%QTDIR%\bin;%PATH%"
start "" "%~dp0..\build\Debug\dayflow.exe"
