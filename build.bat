@echo off

mkdir build
pushd build
cl.exe /Zi "..\source\main.cpp" user32.lib
popd
