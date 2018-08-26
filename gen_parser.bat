@echo off
pushd %~dp0

cd src\Graphics

echo flexing CBSHDR...
..\..\tools\win_flex_bison\win_flex -o cbshdr_lex.yy.cpp cbshdr.l

echo bisoning CBSHDR...
..\..\tools\win_flex_bison\win_bison cbshdr.y --defines=cbshdr.tab.h -o cbshdr.tab.cpp

echo flexing CBMAT...
..\..\tools\win_flex_bison\win_flex -o cbmat_lex.yy.cpp cbmat.l

echo bisoning CBSHDR...
..\..\tools\win_flex_bison\win_bison cbmat.y --defines=cbmat.tab.h -o cbmat.tab.cpp

popd %~dp0
