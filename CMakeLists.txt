cmake_minimum_required(VERSION 3.22)
project(baremetal_workqueue)

enable_language(C ASM)

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Wextra -Wpedantic)

include(cmake/extensions.cmake)

if(NOT BUILD_TARGET STREQUAL "unit_test")
    add_executable(app)
endif()

add_subdirectory(src)
