@echo off
rem Builds the wasm size report and opens it in the browser (Windows).
rem An optional argument overrides the .wasm path. Without an argument it looks
rem for Bin\WebAssembly\*.wasm in the o2 root and, when o2 is a submodule, in
rem the parent project root.
setlocal enabledelayedexpansion
set "TOOL_DIR=%~dp0"
set "O2_ROOT=%TOOL_DIR%..\.."

set "WASM=%~1"
if "%WASM%"=="" (
    for %%R in ("%O2_ROOT%" "%O2_ROOT%\..") do (
        if "!WASM!"=="" (
            for %%F in ("%%~R\Bin\WebAssembly\*.wasm") do if "!WASM!"=="" set "WASM=%%~fF"
        )
    )
)

if "%WASM%"=="" goto :notfound
if not exist "%WASM%" goto :notfound

set "REPORT=%WASM:.wasm=%.size-report.html"
python "%TOOL_DIR%wasm_size_report.py" "%WASM%" -o "%REPORT%"
if errorlevel 1 (
    pause
    exit /b 1
)

start "" "%REPORT%"
exit /b 0

:notfound
echo No .wasm found (looked for Bin\WebAssembly\*.wasm near o2).
echo Build the wasm target first or pass a path: %~nx0 path\to\App.wasm
pause
exit /b 1
