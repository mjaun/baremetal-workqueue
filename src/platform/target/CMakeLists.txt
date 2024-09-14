app_include_directories(
    stm32cube/Drivers/CMSIS/Include
    stm32cube/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    stm32cube/Drivers/STM32F4xx_HAL_Driver/Inc
    stm32cube/Core/Inc
)

app_compile_definitions(STM32F411xE)

app_linker_script(stm32cube/STM32F411RETX_FLASH.ld)

app_link_options(
    --specs=nano.specs
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

file(GLOB core_sources stm32cube/Core/Src/*.c)
file(GLOB hal_sources stm32cube/Drivers/STM32F4xx_HAL_Driver/Src/*.c)

app_sources(
    drivers/uart_stm32.c
    drivers/gpio_stm32.c
    drivers/system_stm32.c
    peripherals.c
    ${core_sources}
    ${hal_sources}
    stm32cube/Core/Startup/startup_stm32f411retx.s
)

app_source_compile_options(
    SOURCE stm32cube/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    COMPILE_OPTIONS -Wno-unused-parameter
)
