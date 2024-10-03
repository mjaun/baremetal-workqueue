app_include_directories(${CMAKE_SOURCE_DIR}/include/common)
test_library_include_directories(${CMAKE_SOURCE_DIR}/include/common)

app_sources(
    application/application_main.c
    service/work.c
    service/log.c
    service/cbprintf.c
    service/assert.c
)

test_library_sources(
    service/work.c
    service/log.c
    service/cbprintf.c
    service/assert.c
)

set(TEST_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/unit_test)

test_define(cbprintf)

test_sources(
    ${TEST_SOURCE_DIR}/service/test_cbprintf.cpp
)
