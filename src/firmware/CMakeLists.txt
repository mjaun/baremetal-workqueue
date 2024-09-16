app_include_directories(
    ${CMAKE_SOURCE_DIR}/include/firmware
    stm32cube/Drivers/CMSIS/Include
    stm32cube/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    stm32cube/Drivers/STM32F4xx_HAL_Driver/Inc
    stm32cube/Core/Inc
)

app_compile_definitions(STM32F411xE)

get_filename_component(LINKER_SCRIPT stm32cube/STM32F411RETX_FLASH.ld ABSOLUTE)
target_link_options(app PRIVATE -T ${LINKER_SCRIPT})
set_property(TARGET app APPEND PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})

target_link_options(app PRIVATE
    --specs=nano.specs
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

file(GLOB core_sources stm32cube/Core/Src/*.c)
file(GLOB hal_sources stm32cube/Drivers/STM32F4xx_HAL_Driver/Src/*.c)

app_sources(
    driver/uart_stm32.c
    driver/gpio_stm32.c
    service/system_stm32.c
    application/peripherals_stm32.c
    ${core_sources}
    ${hal_sources}
    stm32cube/Core/Startup/startup_stm32f411retx.s
)

set_property(
    SOURCE stm32cube/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    DIRECTORY ${CMAKE_SOURCE_DIR}
    APPEND PROPERTY COMPILE_OPTIONS -Wno-unused-parameter
)
