cmake_minimum_required(VERSION 2.8)

project(nolli)

#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)

add_definitions(-Wall)# -Weverything -pedantic)

# Uncomment the next line to enable lots of debug output during runtime
add_definitions(-DDEBUG)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/ext")
include(UseLemon)

find_package(FLEX REQUIRED)

MAKE_LEMON(ext/lemon.c)
LEMON_TARGET(parser grammar.y ext/lempar.c)
FLEX_TARGET(scanner lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.yy.c
    COMPILE_FLAGS --header-file=${CMAKE_CURRENT_BINARY_DIR}/lexer.yy.h)
ADD_FLEX_LEMON_DEPENDENCY(scanner parser)

set(NOLLI_EXE nolli)

set(NOLLI_SOURCES
    main.c
    alloc.c
    type.c
    symtable.c
    ast.c
    ${LEMON_parser_SOURCE}
    ${FLEX_scanner_OUTPUTS}
)
set(NOLLI_HEADERS
    nolli.h
    alloc.h
    symtable.h
    ast.h
)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
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
