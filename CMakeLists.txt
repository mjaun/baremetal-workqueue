cmake_minimum_required(VERSION 3.22)
project(baremetal_workqueue)

enable_language(C ASM)

if(BUILD_TARGET STREQUAL "unit_test")
    enable_testing()
endif()

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Wextra -Wpedantic)

include(cmake/extensions.cmake)

if(BUILD_TARGET STREQUAL "firmware")
    enable_app_build()
    add_subdirectory(src/firmware)
    add_subdirectory(src/common)
endif()

if(BUILD_TARGET STREQUAL "simulator")
    enable_app_build()
    add_subdirectory(src/simulator)
    add_subdirectory(src/common)
endif()

if(BUILD_TARGET STREQUAL "unit_test")
    enable_test_build()
    add_subdirectory(src/unit_test)
    add_subdirectory(src/common)
endif()
