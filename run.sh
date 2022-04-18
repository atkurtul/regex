#!/usr/bin/bash


mkdir -p build
cd build
cmake .. -GNinja
cmake --build .
cd ..
./build/regex
dot -Tpng dfa.dot -o dfa.png