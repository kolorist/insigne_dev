@echo off

if not exist "clang64_rel" (
echo Please run gen_prj_clang_release.bat first!!!
exit /b
)

pushd %~dp0
cd clang64_rel
call cmake --build .
popd