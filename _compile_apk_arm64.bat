@echo off
setlocal EnableDelayedExpansion
if [%1]==[] (
set BUILD_CONFIG=Debug
set TARGET_DIRECTORY=debug_arm64
) else (
set BUILD_CONFIG=%1
set TARGET_DIRECTORY=release_arm64
)

echo Copying .so file
copy %TARGET_DIRECTORY%\libinsigne_dev.so externals\android\app\lib\arm64-v8a\libinsigne_dev.so

pushd %~dp0
cd externals\android
gradlew assembleDebug
popd