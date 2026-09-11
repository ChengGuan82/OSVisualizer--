@echo off
cd /d "%~dp0"
if exist "Release\v1.0\OSVisualizer.exe" (
    start "" "Release\v1.0\OSVisualizer.exe"
) else (
    echo Please run scripts\build.ps1 first. See README.md.
    pause
)
