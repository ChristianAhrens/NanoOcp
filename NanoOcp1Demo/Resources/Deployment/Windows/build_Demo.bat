@echo off
setlocal

:: Navigate from NanoOcp1Demo\Resources\Deployment\Windows\ to the repo root.
pushd "%~dp0..\..\..\.."

echo === NanoOcp1Demo - CMake build (Windows) ===
echo Working directory: %CD%

cmake -B build -S . -DNANOOCP1_BUILD_DEMO=ON
if errorlevel 1 goto :fail

cmake --build build --config Release
if errorlevel 1 goto :fail

echo === Build complete ===
popd
exit /b 0

:fail
echo === Build FAILED ===
popd
exit /b 1
