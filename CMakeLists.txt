cmake_minimum_required(VERSION 4.0.0)
project(RPC VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) #强制要求编译器必须支持你所指定的 C++ 标准（如 C++17），否则立即报错停止生成。

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(test/protobuf/)
add_subdirectory(src/)
add_subdirectory(example/)