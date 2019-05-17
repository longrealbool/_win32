@echo off

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community
REM I don't know why it doesn't work, assume problem with "%LIB%" - syntax
REM IF NOT DEFINED "%LIB%" (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64))

REM work in my case, never been tested. PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
REM OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.

if "%LIB%"=="" (call chcp 437) 
if "%LIB%"=="" (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64))

set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DGAME_INTERNAL=1 -DGAME_SLOW=1 -DGAME_WIN32=1 -FC -Z7
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