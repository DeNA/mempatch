set(SOURCE_FILES
    main.cpp
    Patcher.cpp
    ChangeString.cpp
    Utility.cpp
    Converter.cpp
    Address.cpp
    LineReader.cpp
    linenoise/linenoise.cpp
    FreezeThread.cpp
    SnappedRange.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "android")
    list(APPEND SOURCE_FILES
        Memory_Linux.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(APPEND SOURCE_FILES
        Memory_Darwin.mm
    )
endif()

add_executable(mempatch ${SOURCE_FILES})

set_target_properties(mempatch PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)
target_link_libraries(mempatch PUBLIC
    cxx_flags
    Threads::Threads)