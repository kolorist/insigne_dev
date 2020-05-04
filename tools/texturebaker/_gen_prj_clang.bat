@echo off

if not exist "clang64" (
mkdir clang64
)

pushd %~dp0
cd clang64
call cmake -DCMAKE_C_COMPILER:PATH="C:\Program Files\LLVM\bin\clang.exe" -DCMAKE_CXX_COMPILER:PATH="C:\Program Files\LLVM\bin\clang++.exe"  -DCMAKE_RC_COMPILER:PATH="C:Program Files\LLVM\bin\llvm-rc.exe" -DCMAKE_MAKE_PROGRAM="C:\DevTools\ninja\ninja.exe" -DTARGET_PLATFORM="x64" -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -G Ninja ..
popd