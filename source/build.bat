@echo off
setlocal
if [%1] == [release] (
    set Configuration=/p:Configuration=Release
    shift
)
if [%1] == [Release] (
    set Configuration=/p:Configuration=Release
    shift
)
if [%1] == [] (
    set Target=/target:compile
) else (
    set Target=/target:%1
)
call "%VS100COMNTOOLS%\vsvars32.bat"

msbuild %Configuration% %Target% dwrite.msbuild
endlocal