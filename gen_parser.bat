@echo off
pushd %~dp0

cd src\Graphics

echo flexing...
..\..\tools\win_flex_bison\win_flex -o lex.yy.cpp cbshdr.l

echo bisoning...
..\..\tools\win_flex_bison\win_bison cbshdr.y --defines=cbshdr.tab.h -o cbshdr.tab.cpp

popd %~dp0
