function(enable_app_build)
    add_executable(app)
    set_property(TARGET app PROPERTY OUTPUT_NAME ${CMAKE_PROJECT_NAME})
    set_property(GLOBAL PROPERTY BUILD_APP ON)
endfunction()

function(enable_test_build)
    add_library(test_lib STATIC)
    set_property(GLOBAL PROPERTY BUILD_TESTS ON)
endfunction()

function(app_include_directories)
    get_property(BUILD_APP GLOBAL PROPERTY BUILD_APP)
    if(BUILD_APP)
        target_include_directories(app PRIVATE ${ARGN})
    endif()
endfunction()

function(app_compile_definitions)
    get_property(BUILD_APP GLOBAL PROPERTY BUILD_APP)
    if(BUILD_APP)
        target_compile_definitions(app PRIVATE ${ARGN})
    endif()
endfunction()

function(app_sources)
    get_property(BUILD_APP GLOBAL PROPERTY BUILD_APP)
    if(BUILD_APP)
        target_sources(app PRIVATE ${ARGN})

        foreach(SOURCE_FILE ${ARGN})
            get_filename_component(SOURCE_NAME ${SOURCE_FILE} NAME)

            set_property(
                SOURCE ${SOURCE_FILE}
                DIRECTORY ${CMAKE_SOURCE_DIR}
                APPEND PROPERTY COMPILE_DEFINITIONS __FILENAME__=\"${SOURCE_NAME}\"
            )
        endforeach()
    endif()
endfunction()

function(test_define NAME)
    get_property(BUILD_TESTS GLOBAL PROPERTY BUILD_TESTS)
    if(BUILD_TESTS)
        add_executable(test_${NAME} ${ARGN})
        target_link_libraries(test_${NAME} PRIVATE test_lib)
        add_test(NAME ${NAME} COMMAND $<TARGET_FILE:test_${NAME}> -v)

        foreach(SOURCE_FILE ${ARGN})
            get_filename_component(SOURCE_NAME ${SOURCE_FILE} NAME)

            set_property(
                SOURCE ${SOURCE_FILE}
                DIRECTORY ${CMAKE_SOURCE_DIR}
                APPEND PROPERTY COMPILE_DEFINITIONS __FILENAME__=\"${SOURCE_NAME}\"
            )
        endforeach()
    endif()
endfunction()

function(test_library_include_directories)
    get_property(BUILD_TESTS GLOBAL PROPERTY BUILD_TESTS)
    if(BUILD_TESTS)
        target_include_directories(test_lib PUBLIC ${ARGN})
    endif()
endfunction()

function(test_library_sources)
    get_property(BUILD_TESTS GLOBAL PROPERTY BUILD_TESTS)
    if(BUILD_TESTS)
        target_sources(test_lib PRIVATE ${ARGN})

        foreach(SOURCE_FILE ${ARGN})
            get_filename_component(SOURCE_NAME ${SOURCE_FILE} NAME)

            set_property(
                SOURCE ${SOURCE_FILE}
                DIRECTORY ${CMAKE_SOURCE_DIR}
                APPEND PROPERTY COMPILE_DEFINITIONS __FILENAME__=\"${SOURCE_NAME}\"
            )
        endforeach()
    endif()
endfunction()
