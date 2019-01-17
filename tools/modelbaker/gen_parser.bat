@echo off
pushd %~dp0

cd src

echo flexing CBOBJ...
..\..\win_flex_bison\win_flex -o pbrtv3.yy.cpp pbrtv3.l

echo bisoning CBOBJ...
..\..\win_flex_bison\win_bison pbrtv3.y --defines=pbrtv3.tab.h -o pbrtv3.tab.cpp

popd %~dp0
