cmake_minimum_required(VERSION 3.22)

project(regex)

add_executable(regex label.cpp cc.cpp pda.cpp)
set_property(TARGET regex PROPERTY CXX_STANDARD 20)
