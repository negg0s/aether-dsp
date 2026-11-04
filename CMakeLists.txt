cmake_minimum_required(VERSION 3.20)
project(aether-dsp
    VERSION 1.1.0
    DESCRIPTION "Non-Euclidean Hyperbolic Waveguide & Modal Cymatics Acoustic Synthesis Engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimization settings
if(MSVC)
    add_compile_options(/W4 /O2 /permissive- /utf-8 /arch:AVX2 /wd4324)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 1. Main Executable & Benchmark Suite
add_executable(aether-dsp
    src/main.cpp
    src/audio_device.cpp
    src/c_api.cpp
)

# 2. Shared Dynamic Library (C-ABI for Python, FFI, Game Engines)
add_library(aether-dsp-shared SHARED
    src/c_api.cpp
)
set_target_properties(aether-dsp-shared PROPERTIES
    OUTPUT_NAME "aether-dsp"
    DEFINE_SYMBOL "AETHER_BUILD_DLL"
)

# 3. Test Suite Executable
add_executable(aether-tests
    tests/unit_tests.cpp
    src/c_api.cpp
)

enable_testing()
add_test(NAME AetherUnitTests COMMAND aether-tests)

# Installation rules & Package export
include(GNUInstallDirs)
install(TARGETS aether-dsp aether-dsp-shared aether-tests
    EXPORT AetherDSPTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
