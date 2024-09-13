function(app_include_directories)
    target_include_directories(app PRIVATE ${ARGN})
endfunction()

function(app_compile_definitions)
    target_compile_definitions(app PRIVATE ${ARGN})
endfunction()

function(app_compile_options)
    target_compile_options(app PRIVATE ${ARGN})
endfunction()

function(app_sources)
    target_sources(app PRIVATE ${ARGN})
endfunction()

function(app_link_options)
    target_link_options(app PRIVATE ${ARGN})
endfunction()

function(app_linker_script)
    get_filename_component(LINKER_SCRIPT ${ARGV0} ABSOLUTE)
    target_link_options(app PRIVATE -T ${LINKER_SCRIPT})
    set_property(TARGET app APPEND PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})
endfunction()

function(app_source_compile_options)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs SOURCE COMPILE_OPTIONS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set_property(
        SOURCE ${ARG_SOURCE}
        DIRECTORY ${CMAKE_SOURCE_DIR}
        APPEND PROPERTY COMPILE_OPTIONS ${ARG_COMPILE_OPTIONS}
    )
endfunction()
