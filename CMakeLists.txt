cmake_minimum_required(VERSION 2.8)

project(nolli)

add_definitions(-Wall)# -Weverything -pedantic)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Default to Debug Build Type" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

set(NOLLI_NAME nolli)

set(NOLLI_SOURCES
    init.c
    alloc.c
    lexer.c
    parser.c
    ast.c
    ast_graph.c
    walk.c
    symtable.c
    type.c
    os.c
)

set(NOLLI_HEADERS
    nolli.h
    error.h
    alloc.h
    lexer.h
    parser.h
    ast.h
    ast_graph.h
    walk.h
    symtable.h
    type.h
    os.h
)

add_library(nolli ${NOLLI_SOURCES})
# link only against math library
target_link_libraries(nolli m)
install(TARGETS nolli DESTINATION lib)

add_executable(nolli-exe main.c linenoise/linenoise.c)
set_target_properties(nolli-exe PROPERTIES OUTPUT_NAME nolli)
target_link_libraries(nolli-exe nolli)
install(TARGETS nolli-exe DESTINATION bin)

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
