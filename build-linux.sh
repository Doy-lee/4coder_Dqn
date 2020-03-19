#!/bin/bash

pushd custom/Dqn
../bin/buildsuper_x64-linux.sh 4coder_Dqn.cpp
cp custom_4coder.so ../../custom_4coder.so
popd
