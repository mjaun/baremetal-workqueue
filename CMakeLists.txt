cmake_minimum_required(VERSION 3.22)
project(baremetal_workqueue)

enable_language(C ASM)

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Wextra -Wpedantic)

include(cmake/extensions.cmake)

add_executable(app)

app_include_directories(include)

add_subdirectory(src/platform)
add_subdirectory(src/application)
