if(BUILD_TARGET STREQUAL "firmware")
    add_subdirectory(common)
    add_subdirectory(firmware)
endif()

if(BUILD_TARGET STREQUAL "simulator")
    add_subdirectory(common)
    add_subdirectory(simulator)
endif()

if(BUILD_TARGET STREQUAL "unit_test")
    add_subdirectory(unit_test)
endif()
