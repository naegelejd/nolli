cmake_minimum_required(VERSION 2.8)

project(nolli)

#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)

add_definitions(-Wall)# -Weverything -pedantic)

# Uncomment the next line to enable lots of debug output during runtime
add_definitions(-DDEBUG)

set(NOLLI_EXE nolli)

set(NOLLI_SOURCES
    main.c
    alloc.c
    lexer.c
    parser.c
    ast.c
)

set(NOLLI_HEADERS
    nolli.h
    error.h
    alloc.h
    lexer.h
    parser.h
    ast.h
)

#include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_executable(${NOLLI_EXE} ${NOLLI_SOURCES})

# link only against math library
target_link_libraries(${NOLLI_EXE} m)

install(TARGETS ${NOLLI_EXE} DESTINATION bin)

set_source_files_properties(tags PROPERTIES GENERATED true)
add_custom_command(
    OUTPUT tags
    COMMAND ctags ${NOLLI_SOURCES} ${NOLLI_HEADERS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
add_custom_target(ctags DEPENDS tags)

#enable_testing()
#add_subdirectory(tests)

find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_subdirectory(doc)
else(DOXYGEN_FOUND)
    message("Doxygen is needed to build nolli's API documentation")
endif(DOXYGEN_FOUND)
