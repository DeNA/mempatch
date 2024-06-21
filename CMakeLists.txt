cmake_minimum_required(VERSION 3.15)

project(MemoryPatching VERSION 1.0)

# android | iOS
set(CMAKE_SYSTEM_NAME android)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cxx_flags INTERFACE)
target_compile_features(cxx_flags INTERFACE cxx_std_14)
target_compile_definitions(cxx_flags INTERFACE
    _FILE_OFFSET_BITS=64
    __IS_NDK_BUILD__=1)

add_subdirectory(jni)