@echo off

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" goto:start)

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" goto:start)

REM work in my case, never been tested. 
REM PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.

:start
IF NOT DEFINED LIB (call chcp 437)
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64))

set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -O2 -Oi -WX -W4 -wd4456 -wd4201 -wd4100 -wd4189 -wd4505 -DGAME_INTERNAL=1 -DGAME_SLOW=1 -DGAME_WIN32=1 -DNO_ASSETS=0 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

REM TODO - can we just build both with one exe?

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

REM 32-bit build
REM cl %CommonCompilerFlags% ..\code\win32_game.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build /O2 /Oi /fp:fast
del *.pdb > NUL 2> NUL
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags%  ..\source\code\game.cpp -Fmhgame.map -LD /link -incremental:no -opt:ref -PDB:game_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
del lock.tmp
cl %CommonCompilerFlags% ..\source\code\win32_game.cpp -Fmwin32_game.map /link %CommonLinkerFlags%
popd


REM if "%LIB%"=="" (call chcp 437) 
REM if "%LIB%"=="" (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64))
REM I don't know why it doesn't work, assume problem with "%LIB%" - syntax
REM IF NOT DEFINED "%LIB%" (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64))