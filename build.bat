@echo off
pushd custom\Dqn
call ..\bin\buildsuper_x64.bat 4coder_Dqn.cpp
copy custom_4coder.dll ..\..\custom_4coder.dll
copy custom_4coder.pdb ..\..\custom_4coder.pdb
popd
