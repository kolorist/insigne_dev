@echo off
pushd %~dp0

cd src

echo flexing CBOBJ...
..\..\win_flex_bison\win_flex -o cbobj_lex.yy.cpp cbobj.l

echo bisoning CBOBJ...
echo ..\tools\win_flex_bison\win_bison cbobj.y --defines=cbobj.tab.h -o cbobj.tab.cpp

popd %~dp0
