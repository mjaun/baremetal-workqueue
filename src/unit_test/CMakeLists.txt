include(FetchContent)

FetchContent_Declare(
    CppUTest
    GIT_REPOSITORY https://github.com/cpputest/cpputest.git
    GIT_TAG        master # or use release tag, eg. v4.0
)

set(TESTS OFF CACHE BOOL "Disable CppUTest tests")
set(CPPUTEST_MEM_LEAK_DETECTION_DISABLED ON)

FetchContent_MakeAvailable(CppUTest)

target_link_libraries(test_lib PUBLIC CppUTest::CppUTest CppUTest::CppUTestExt)

test_library_include_directories(${CMAKE_SOURCE_DIR}/include/unit_test)

test_library_sources(
    service/system_fake.c
    main.cpp
)
