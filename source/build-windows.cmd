@echo off
setlocal
cd /d "%~dp0"
python build.py
if errorlevel 1 exit /b 1
echo Build complete.
