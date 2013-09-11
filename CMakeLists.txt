cmake_minimum_required(VERSION 2.8)

project(nolli)

#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)

add_definitions(-Wall)# -Weverything -pedantic)

# Uncomment the next line to enable lots of debug output during runtime
#add_definitions(-DNDEBUG -DDEBUG)

set(NOLLI_EXE_NAME nolli)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

#BISON_TARGET(myparser parser.y ${CMAKE_CURRENT_SOURCE_DIR}/parser.tab.c VERBOSE parser.output COMPILE_FLAGS -d)
BISON_TARGET(myparser parser.y ${CMAKE_CURRENT_SOURCE_DIR}/parser.tab.c COMPILE_FLAGS -d)
FLEX_TARGET(myscanner lexer.l ${CMAKE_CURRENT_SOURCE_DIR}/lexer.yy.c)
ADD_FLEX_BISON_DEPENDENCY(myscanner myparser)

set(NOLLI_SOURCES
    main.c
    alloc.c
    symtable.c
    ${BISON_myparser_OUTPUTS}
    ${FLEX_myscanner_OUTPUTS}
)
set(NOLLI_HEADERS
    nolli.h
    alloc.h
    symtable.h
)

add_executable(${NOLLI_EXE_NAME} ${NOLLI_SOURCES})

# link only against math library
target_link_libraries(${NOLLI_EXE_NAME} m)

install(TARGETS ${NOLLI_EXE_NAME} DESTINATION bin)

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
