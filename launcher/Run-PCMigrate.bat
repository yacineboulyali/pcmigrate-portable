@echo off
rem ============================================================
rem  PC Migrate Portable - launcher
rem  Works from a USB stick, from Hiren's BootCD PE and from Windows.
rem  PCMigrate.exe asks for administrator rights by itself (UAC).
rem ============================================================
setlocal
cd /d "%~dp0"
if not exist "%~dp0PCMigrate.exe" (
    echo [ERROR] PCMigrate.exe was not found next to this file.
    echo Put Run-PCMigrate.bat and PCMigrate.exe in the same folder.
    pause
    exit /b 1
)
reg query "HKLM\SYSTEM\CurrentControlSet\Control\MiniNT" >nul 2>&1
if %errorlevel%==0 (
    echo Running in WinPE / Hiren's BootCD PE - offline mode.
) else (
    echo Running on Windows - a UAC prompt may appear.
)
start "" "%~dp0PCMigrate.exe"
exit /b 0
