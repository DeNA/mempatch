cmake_minimum_required(VERSION 3.15)

project(MemoryPatching VERSION 1.0)

# android | iOS | Windows
set(CMAKE_SYSTEM_NAME android)

add_library(cxx_flags INTERFACE)
target_compile_features(cxx_flags INTERFACE cxx_std_14)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
target_compile_definitions(cxx_flags INTERFACE
    _FILE_OFFSET_BITS=64
    NOMINMAX)
else()
target_compile_definitions(cxx_flags INTERFACE
    _FILE_OFFSET_BITS=64
    __IS_NDK_BUILD__=1)
endif()
add_subdirectory(jni)