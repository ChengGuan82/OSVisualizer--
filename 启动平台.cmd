@echo off
cd /d "%~dp0"
if exist "Release\v2.1\OSVisualizer.exe" (
    start "" "Release\v2.1\OSVisualizer.exe"
) else (
    echo Please run scripts\build.ps1 first. See README.md.
    pause
)
