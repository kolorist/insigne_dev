@echo off
setlocal EnableDelayedExpansion
if [%1]==[] (
set BUILD_CONFIG=Debug
) else (
set BUILD_CONFIG=%1
)

if not exist "clang64" (
echo Please run gen_prj_clang.bat first!!!
exit /b
)

pushd %~dp0
cd clang64
call cmake --build . --config %BUILD_CONFIG%
popd