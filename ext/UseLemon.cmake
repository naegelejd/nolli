macro(MAKE_LEMON source)
    set(LEMON_EXE lemon)
    add_executable(${LEMON_EXE} ${source})
endmacro()

macro(LEMON_TARGET name grammar template)
    string(REGEX REPLACE "^(.*)\\.([^.]*)$" "\\1.c" "LEMON_${name}_SOURCE" ${grammar})
    string(REGEX REPLACE "^(.*)\\.([^.]*)$" "\\1.h" "LEMON_${name}_HEADER" ${grammar})

    add_custom_command(
        OUTPUT ${LEMON_${name}_SOURCE} ${LEMON_${name}_HEADER}
        COMMAND ${CMAKE_COMMAND} -E copy ${grammar} ${CMAKE_CURRENT_BINARY_DIR}/${grammar}
        COMMAND ${LEMON_EXE} -q T=${template} ${CMAKE_CURRENT_BINARY_DIR}/${grammar}
        DEPENDS ${LEMON_EXE} ${grammar}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating parser from ${grammar} using ${LEMON_EXE}"
        VERBATIM)

endmacro()

macro(ADD_FLEX_LEMON_DEPENDENCY flex_target lemon_target)
    if (NOT FLEX_${flex_target}_OUTPUTS)
        message(SEND_ERROR "Flex target `${flex_target}` does not exist.")
    endif()

    if (NOT LEMON_${lemon_target}_HEADER)
        message(SEND_ERROR "Lemon target `${lemon_target}` does not exist.")
    endif()

    set_source_files_properties(${FLEX_${flex_target}_OUTPUTS}
        PROPERTIES OBJECT_DEPENDS ${LEMON_${lemon_target}_HEADER})
endmacro()
