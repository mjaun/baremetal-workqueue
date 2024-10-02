include(FetchContent)

FetchContent_Declare(
    cJSON
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG        master # or use release tag, eg. v4.0
)

set(BUILD_SHARED_LIBS OFF)
set(ENABLE_CJSON_TEST OFF)
set(ENABLE_CJSON_UNINSTALL OFF)

FetchContent_MakeAvailable(cJSON)

target_link_libraries(app PRIVATE cjson)

# unfortunately cJSON doesn't provide include directories via CMake
get_target_property(CJSON_SOURCE_DIR cjson SOURCE_DIR)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/cjson)
file(CREATE_LINK ${CJSON_SOURCE_DIR}/cJSON.h ${CMAKE_CURRENT_BINARY_DIR}/include/cjson/cJSON.h SYMBOLIC)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

app_include_directories(${CMAKE_SOURCE_DIR}/include/simulator)

app_sources(
    application/peripherals_sim.c
    driver/uart_sim.c
    driver/gpio_sim.c
    service/system_sim.c
    service/adapter_sim.c
    main.c
)
