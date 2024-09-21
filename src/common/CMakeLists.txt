app_include_directories(${CMAKE_SOURCE_DIR}/include/common)

app_sources(
    application/application_startup.c
    application/uptime_printer.c
    application/button_handler.c
    service/work.c
    service/log.c
)

set(TEST_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/unit_test)

test_define(uptime_printer)

test_sources(
    application/uptime_printer.c
    ${TEST_SOURCE_DIR}/application/test_uptime_printer.cpp
)
