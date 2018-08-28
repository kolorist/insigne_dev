@echo off
pushd %~dp0

cd src\Graphics

echo flexing CBSHDR...
..\..\tools\win_flex_bison\win_flex -o cbshdr_lex.yy.cpp cbshdr.l

echo bisoning CBSHDR...
..\..\tools\win_flex_bison\win_bison cbshdr.y --defines=cbshdr.tab.h -o cbshdr.tab.cpp

popd %~dp0
