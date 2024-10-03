cmake_minimum_required(VERSION 3.22)
project(baremetal_workqueue)

enable_language(C ASM)

if(BUILD_TARGET STREQUAL "unit_test")
    enable_language(CXX)
    enable_testing()
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_options(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=PROJECT_ROOT)
add_compile_options(-fstack-usage)  # for puncover

include(cmake/extensions.cmake)

if(BUILD_TARGET STREQUAL "firmware")
    enable_app_build()
    add_compile_definitions(BUILD_FIRMWARE)
    add_subdirectory(src/firmware)
    add_subdirectory(src/common)
endif()

if(BUILD_TARGET STREQUAL "simulator")
    enable_app_build()
    add_compile_definitions(BUILD_SIMULATOR)
    add_subdirectory(src/simulator)
    add_subdirectory(src/common)
endif()

if(BUILD_TARGET STREQUAL "unit_test")
    enable_test_build()
    add_compile_definitions(BUILD_UNIT_TEST)
    add_subdirectory(src/unit_test)
    add_subdirectory(src/common)
endif()
