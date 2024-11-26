set(SOURCE_FILES
    main.cpp
    Patcher.cpp
    ChangeString.cpp
    Utility.cpp
    Converter.cpp
    Address.cpp
    FreezeThread.cpp
    SnappedRange.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    list(APPEND SOURCE_FILES
        Memory_Linux.cpp
        linenoise/linenoise.cpp
        LineReader.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(APPEND SOURCE_FILES
        Memory_Darwin.mm
        linenoise/linenoise.cpp
        LineReader.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND SOURCE_FILES
        getopt.cpp
        Memory_Windows.cpp
        LineReader_Windows.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND SOURCE_FILES
        Memory_Linux.cpp
        linenoise/linenoise.cpp
        LineReader.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND SOURCE_FILES
        Memory_Darwin.mm
        linenoise/linenoise.cpp
        LineReader.cpp
    )
endif()

add_executable(mempatch ${SOURCE_FILES})

set_target_properties(mempatch PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
target_link_libraries(mempatch PUBLIC
    cxx_flags
    Threads::Threads
    psapi)
else()
target_link_libraries(mempatch PUBLIC
    cxx_flags
    Threads::Threads)
endif()
