add_executable(mempatch
    main.cpp
    Patcher.cpp
    ChangeString.cpp
    Ptrace.cpp
    Utility.cpp
    Converter.cpp
    Address.cpp
    LineReader.cpp
    linenoise/linenoise.cpp
    FreezeThread.cpp
    SnappedRange.cpp)

set_target_properties(mempatch PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)
target_link_libraries(mempatch PUBLIC
    cxx_flags
    Threads::Threads)