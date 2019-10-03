@echo off
cd C:\_asm\projects\_win32
git pull origin master
cd source\code
CALL build.bat
start 4ed game.cpp
start devenv ../../build/win32_game.sln
cmd /k