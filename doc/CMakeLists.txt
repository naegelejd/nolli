set(DOXYGEN_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/html/index.html)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# configure Doxyfile.in, generating Doxyfile
configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)

add_custom_command(
    OUTPUT ${DOXYGEN_OUTPUT}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API Documentation with Doxygen..." VERBATIM
    DEPENDS ${DOXYFILE}
)

add_custom_target(doc DEPENDS ${DOXYGEN_OUTPUT})
