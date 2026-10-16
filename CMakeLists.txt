cmake_minimum_required(VERSION 3.25)

project(gs LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/SDL EXCLUDE_FROM_ALL SYSTEM)

add_executable(gs src/main.c)
target_link_libraries(gs PRIVATE SDL3::SDL3-static)

if(MSVC)
    target_compile_options(gs PRIVATE /W4 /WX)
else()
    target_compile_options(gs PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wformat=2
        -Wundef
        -Wcast-qual
        -Wwrite-strings
        -Wstrict-prototypes
        -Wmissing-prototypes
        -Wold-style-definition
        -Werror
    )

    set(SANITIZERS address,undefined)
    target_compile_options(gs PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=${SANITIZERS} -fno-omit-frame-pointer -fno-sanitize-recover=all>
    )
    target_link_options(gs PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=${SANITIZERS}>
    )
endif()
