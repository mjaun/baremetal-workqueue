add_subdirectory(common)

if(TARGET_BUILD)
    add_subdirectory(target)
endif()

if(SIMULATOR_BUILD)
    add_subdirectory(simulator)
endif()
