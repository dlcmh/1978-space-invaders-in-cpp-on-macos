# Compile each portable header as its own translation unit so it cannot rely on
# include order or another header's transitive includes. This uses the normal
# compiler and warning set, so no extra tooling is required.

function(space_invaders_add_header_checks target_name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "HEADERS;INCLUDE_DIRS")
    if(NOT ARG_HEADERS)
        message(FATAL_ERROR "space_invaders_add_header_checks: HEADERS is required")
    endif()

    set(header_sources)
    foreach(header IN LISTS ARG_HEADERS)
        get_filename_component(stem "${header}" NAME_WE)
        set(source "${CMAKE_CURRENT_BINARY_DIR}/header-checks/${target_name}/${stem}.cpp")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/header-checks/${target_name}")
        file(GENERATE OUTPUT "${source}" CONTENT "#include <${header}>\n")
        list(APPEND header_sources "${source}")
    endforeach()

    add_library(${target_name} OBJECT ${header_sources})
    target_include_directories(${target_name} PRIVATE ${ARG_INCLUDE_DIRS})
    target_link_libraries(${target_name} PRIVATE spaceinvaders_warnings)
    set_target_properties(
        ${target_name}
        PROPERTIES
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
    )
endfunction()
