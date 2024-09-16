app_include_directories(${CMAKE_SOURCE_DIR}/include/simulator)

app_sources(
    application/peripherals.c
    driver/uart_sim.c
    driver/gpio_sim.c
    service/system_sim.c
)
