@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
mkdir c:\github\HMH\build
pushd c:\github\HMH\build
cl -FAsc -Zi c:\github\HMH\code\win32_handmade.cpp user32.lib Gdi32.lib
popd