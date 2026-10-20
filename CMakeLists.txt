cmake_minimum_required(VERSION 3.28)

project(
    SpaceInvaders1978
    VERSION 1.0.0
    DESCRIPTION "A native Metal tribute to Taito's 1978 Space Invaders"
    LANGUAGES CXX OBJCXX
)

if(NOT APPLE)
    message(FATAL_ERROR "This project targets macOS: Metal and AppKit are required.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJCXX_STANDARD 23)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version" FORCE)
endif()

option(SPACE_INVADERS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(SPACE_INVADERS_BUILD_APP "Build the macOS application" ON)
option(SPACE_INVADERS_BUILD_CHECKS "Build the headless simulation checks" ON)

add_library(spaceinvaders_warnings INTERFACE)
target_compile_options(
    spaceinvaders_warnings
    INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wformat=2
        -Wundef
        -Wnull-dereference
        $<$<BOOL:${SPACE_INVADERS_WARNINGS_AS_ERRORS}>:-Werror>
)
target_compile_options(
    spaceinvaders_warnings
    INTERFACE
        $<$<COMPILE_LANGUAGE:OBJCXX>:-Wno-direct-ivar-access>
)

add_subdirectory(Sources/Core)
add_subdirectory(Sources/Render)

if(SPACE_INVADERS_BUILD_APP)
    add_subdirectory(Sources/App)
endif()

if(SPACE_INVADERS_BUILD_CHECKS)
    enable_testing()
    add_subdirectory(Checks)
endif()
