@echo off

if not exist build mkdir build
pushd build
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
cl.exe /W3 -FC -Zi -nologo "..\source\main.cpp" user32.lib gdi32.lib
popd
:: cl -some_preprocessor_param=1
