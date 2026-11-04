@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" 10.0.26100.0
set "PATH=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64;C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64;C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
set CXX=cl.exe
set CC=cl.exe
cd /d D:\Projects\aether-dsp
cmake --build build --config Release
endlocal
