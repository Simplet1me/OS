@echo off
rem ============================================================
rem  Build OS course experiments with MinGW g++ (Release)
rem  (requires g++ in PATH)
rem ============================================================
cmake --preset mingw
if errorlevel 1 exit /b 1
cmake --build --preset mingw-release
if errorlevel 1 exit /b 1
echo.
echo Build OK:
echo   exp  : build\mingw\exp_deadlock.exe
echo   test : build\mingw\test_banker.exe
