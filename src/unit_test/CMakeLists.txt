include(FetchContent)

FetchContent_Declare(
    CppUTest
    GIT_REPOSITORY https://github.com/cpputest/cpputest.git
    GIT_TAG        master # or use release tag, eg. v4.0
)

set(TESTS OFF CACHE BOOL "Disable CppUTest tests")

FetchContent_MakeAvailable(CppUTest)

add_executable(unit_test)

target_include_directories(unit_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include/common
    ${CMAKE_SOURCE_DIR}/include/unit_test
)

target_sources(unit_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common/application/uptime_printer.c
    ${CMAKE_SOURCE_DIR}/src/common/service/work.c
    application/test_uptime_printer.cpp
    application/peripherals_fake.cpp
    driver/uart_mock.cpp
    service/system_fake.cpp
    service/work_ext.c
)

target_link_libraries(unit_test CppUTest::CppUTest CppUTest::CppUTestExt)
