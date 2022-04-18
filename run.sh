#!/usr/bin/bash


mkdir -p build
cd build
cmake .. -GNinja
cmake --build .
cd ..
./build/regex
