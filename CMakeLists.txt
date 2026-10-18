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

if(BUILD_TESTING)
    find_package(Catch2 3.16.1 CONFIG QUIET)

    if(NOT TARGET Catch2::Catch2WithMain)
        include(FetchContent)

        FetchContent_Declare(
            Catch2
            URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.16.1.tar.gz
            URL_HASH SHA256=d16bd1b8b2364918bd472cb216394acb93bde6784890df0d91bccfb3f28d777d
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(Catch2)
    endif()

    include(Catch)
endif()

option(SIGNAL_SIMULATOR_ENABLE_CLANG_TIDY "Run clang-tidy during C++ compilation" OFF)
if(SIGNAL_SIMULATOR_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
    set(SIGNAL_SIMULATOR_CLANG_TIDY_COMMAND
        "${CLANG_TIDY_EXECUTABLE};--experimental-custom-checks"
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(SIGNAL_SIMULATOR_PLATFORM_CPP_SOURCES
        src/platform/LinuxOsAbstractions.cpp
    )
    set(SIGNAL_SIMULATOR_PLATFORM_HEADER_FILES
        src/platform/LinuxOsAbstractions.h
    )
else()
    message(FATAL_ERROR "Unsupported target platform: ${CMAKE_SYSTEM_NAME}")
endif()

set(SIGNAL_SIMULATOR_CPP_SOURCES
    src/Main.cpp
    src/disturbance/FrequencyDisturbanceGenerator.cpp
    src/disturbance/SagDisturbanceGenerator.cpp
    src/disturbance/SurgeDisturbanceGenerator.cpp
    src/disturbance/SwellDisturbanceGenerator.cpp
    src/memory/MemoryHandler.cpp
    src/output/AdcSampleQuantizer.cpp
    src/output/BlockAssembler.cpp
    src/output/Float32SampleQuantizer.cpp
    src/output/OutputGenerator.cpp
    src/signal/SineSignalGenerator.cpp
    ${SIGNAL_SIMULATOR_PLATFORM_CPP_SOURCES}
)
set(SIGNAL_SIMULATOR_HEADER_FILES
    src/config/Config.h
    src/config/ConfigParser.h
    src/disturbance/FrequencyDisturbanceGenerator.h
    src/disturbance/IDisturbanceGenerator.h
    src/disturbance/SagDisturbanceGenerator.h
    src/disturbance/SurgeDisturbanceGenerator.h
    src/disturbance/SwellDisturbanceGenerator.h
    src/memory/MemoryHandler.h
    src/output/AdcSampleQuantizer.h
    src/output/BlockAssembler.h
    src/output/Float32SampleQuantizer.h
    src/output/ISampleQuantizer.h
    src/output/OutputGenerator.h
    src/platform/IOsAbstractions.h
    src/signal/ISignalGenerator.h
    src/signal/SineSignalGenerator.h
    ${SIGNAL_SIMULATOR_PLATFORM_HEADER_FILES}
)
set(SIGNAL_SIMULATOR_CXX_SOURCES
    ${SIGNAL_SIMULATOR_CPP_SOURCES}
    src/config/ConfigParser.cpp
    ${SIGNAL_SIMULATOR_HEADER_FILES}
)

add_library(signal_simulator_config src/config/ConfigParser.cpp src/config/ConfigParser.h)
target_compile_features(signal_simulator_config PUBLIC cxx_std_17)
target_include_directories(signal_simulator_config PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(signal_simulator_config PUBLIC nlohmann_json::nlohmann_json)

add_executable(signal_simulator
    ${SIGNAL_SIMULATOR_CPP_SOURCES}
    ${SIGNAL_SIMULATOR_HEADER_FILES}
)
target_link_libraries(signal_simulator PRIVATE signal_simulator_config)

if(SIGNAL_SIMULATOR_ENABLE_CLANG_TIDY)
    set_target_properties(signal_simulator_config signal_simulator
        PROPERTIES
            CXX_CLANG_TIDY "${SIGNAL_SIMULATOR_CLANG_TIDY_COMMAND}"
    )
endif()

if(BUILD_TESTING)
    set(SIGNAL_SIMULATOR_TEST_SOURCES
        test/TestConfigParser.cpp
    )

    add_executable(signal_simulator_tests ${SIGNAL_SIMULATOR_TEST_SOURCES})
    target_link_libraries(signal_simulator_tests
        PRIVATE
            signal_simulator_config
            Catch2::Catch2WithMain
    )
    catch_discover_tests(signal_simulator_tests
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

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