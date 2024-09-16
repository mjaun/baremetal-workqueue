include(FetchContent)

FetchContent_Declare(
    CppUTest
    GIT_REPOSITORY https://github.com/cpputest/cpputest.git
    GIT_TAG        master # or use release tag, eg. v4.0
)

set(TESTS OFF CACHE BOOL "Disable CppUTest tests")

FetchContent_MakeAvailable(CppUTest)

target_include_directories(test_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include/common
    ${CMAKE_SOURCE_DIR}/include/unit_test
)

target_sources(test_lib PRIVATE
    application/peripherals_fake.c
    driver/uart_mock.cpp
    service/system_fake.c
    service/work_ext.c
    main.cpp

    ${CMAKE_SOURCE_DIR}/src/common/service/work.c
)

target_link_libraries(test_lib PUBLIC CppUTest::CppUTest CppUTest::CppUTestExt)
