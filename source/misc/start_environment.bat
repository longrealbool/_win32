@echo off
cd %~dp0
cd ..\..
git pull origin master
cd source\code
CALL build.bat
start 4ed game.cpp
start devenv ../../build/win32_test.sln
cmd /k