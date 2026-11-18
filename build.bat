@echo off
rem Build PCMigrate.exe on Windows with MSYS2 / MinGW-w64 (g++ and windres in PATH)
cd /d "%~dp0"
if not exist build mkdir build
windres src\app.rc -O coff -o build\app.res || goto :err
g++ -std=c++17 -O2 -s -municode -mwindows -static -static-libgcc -static-libstdc++ src\main.cpp build\app.res -o build\PCMigrate.exe -lcomctl32 -lshell32 -lole32 -luuid -ladvapi32 -lshlwapi -luser32 -lgdi32 || goto :err
echo OK -^> build\PCMigrate.exe
goto :eof
:err
echo BUILD FAILED
exit /b 1
