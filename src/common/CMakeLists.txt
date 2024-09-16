app_include_directories(${CMAKE_SOURCE_DIR}/include/common)

app_sources(
    application/application_startup.c
    application/uptime_printer.c
    application/button_handler.c
    service/work.c
)
