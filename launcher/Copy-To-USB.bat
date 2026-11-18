@echo off
rem ============================================================
rem  Copies PC Migrate Portable to a USB stick (e.g. your Hiren's USB)
rem  Usage: double-click, then type the USB drive letter.
rem ============================================================
setlocal
cd /d "%~dp0"
set /p USB=USB drive letter (example E): 
if "%USB%"=="" goto :eof
set USB=%USB::=%
if not exist "%USB%:\" (
    echo [ERROR] Drive %USB%: not found.
    pause
    exit /b 1
)
set DEST=%USB%:\PCMigrate
if not exist "%DEST%" mkdir "%DEST%"
copy /y "%~dp0PCMigrate.exe" "%DEST%\" >nul || goto :fail
copy /y "%~dp0Run-PCMigrate.bat" "%DEST%\" >nul
if exist "%~dp0README_AR.md" copy /y "%~dp0README_AR.md" "%DEST%\" >nul
echo.
echo Done: %DEST%\PCMigrate.exe
echo Boot Hiren's BootCD PE, open %USB%:\PCMigrate and run Run-PCMigrate.bat
echo (the drive letter can be different inside Hiren's).
pause
exit /b 0
:fail
echo [ERROR] copy failed.
pause
exit /b 1
