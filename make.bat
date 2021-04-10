@echo off

mkdir build
pushd build
:: call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
cl.exe -FC -Zi "..\source\main.cpp" user32.lib gdi32.lib	
popd
