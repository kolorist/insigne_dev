@echo off
setlocal EnableDelayedExpansion
if [%1]==[] (
set BUILD_CONFIG=Debug
set TARGET_DIRECTORY=debug_arm64
) else (
set BUILD_CONFIG=%1
set TARGET_DIRECTORY=release_arm64
)

if not exist "%TARGET_DIRECTORY%" (
echo Please run gen_prj_clang.bat first!!!
exit /b
)

pushd %~dp0
cd %TARGET_DIRECTORY%
call cmake --build . --config %BUILD_CONFIG%
popd