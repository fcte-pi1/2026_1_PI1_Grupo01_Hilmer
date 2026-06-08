@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run_platformio_tests.ps1"
exit /b %ERRORLEVEL%
