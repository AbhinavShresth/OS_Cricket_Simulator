#!/bin/bash

set -e 

if [ -d "build" ]; then
    rm -rf build
fi
mkdir -p build
cd build
cmake ..
cd ..
cmake --build build
./build/cricket_sim
