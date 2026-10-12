cmake_minimum_required(VERSION 3.24)

project(SignalSimulator VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

find_package(nlohmann_json 3.12.0 CONFIG QUIET)

# This project uses nlohmann json parser for config parsing.
if(NOT TARGET nlohmann_json::nlohmann_json)
    include(FetchContent)

    set(JSON_BuildTests OFF CACHE INTERNAL "")
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

option(SIGNAL_SIMULATOR_ENABLE_CLANG_TIDY "Run clang-tidy during C++ compilation" OFF)
if(SIGNAL_SIMULATOR_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
    set(CMAKE_CXX_CLANG_TIDY
        "${CLANG_TIDY_EXECUTABLE};--experimental-custom-checks"
    )
endif()

set(SIGNAL_SIMULATOR_CPP_SOURCES
    src/Main.cpp
    src/MemoryHandler.cpp
    src/OsAbstractions.cpp
)
set(SIGNAL_SIMULATOR_CXX_SOURCES
    ${SIGNAL_SIMULATOR_CPP_SOURCES}
    src/MemoryHandler.h
    src/OsAbstractions.h
)
add_executable(signal_simulator ${SIGNAL_SIMULATOR_CXX_SOURCES})
target_compile_features(signal_simulator PRIVATE cxx_std_17)
target_link_libraries(signal_simulator PRIVATE nlohmann_json::nlohmann_json)

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    add_custom_target(format-check
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror ${SIGNAL_SIMULATOR_CXX_SOURCES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Checking C++ formatting"
        VERBATIM
    )
else()
    add_custom_target(format-check
        COMMAND "${CMAKE_COMMAND}" -E echo "clang-format is required for format-check"
        COMMAND "${CMAKE_COMMAND}" -E false
        COMMENT "Cannot check C++ formatting"
        VERBATIM
    )
endif()

if(CLANG_FORMAT_EXECUTABLE)
    add_dependencies(signal_simulator format-check)
endif()