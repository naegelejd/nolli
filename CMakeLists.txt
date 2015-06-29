cmake_minimum_required(VERSION 2.8)

project(nolli)

# TODO: this is hack to work with Homebrew on OS X
if (NOT DEFINED LLVM_DIR)
    set(LLVM_DIR "/usr/local/Cellar/llvm/3.6.1/share/llvm/cmake")
endif (NOT DEFINED LLVM_DIR)
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs support core mcjit native)

add_definitions(-Wall)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Default to Debug Build Type" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

set(NOLLI_NAME nolli)

set(NOLLI_SOURCES
    nolli.c
    strtab.c
    lexer.c
    parser.c
    ast.c
    graph.c
    symtable.c
    type.c
    analyze.c
    gen.c
)

if (NOT WIN32)
    list (APPEND NOLLI_SOURCES os.c)
endif (NOT WIN32)

add_library(nolli SHARED ${NOLLI_SOURCES})

target_link_libraries(nolli ${llvm_libs})
# link only against math library
if (NOT WIN32)
    target_link_libraries(nolli m)
endif (NOT WIN32)
install(TARGETS nolli DESTINATION lib)
install(FILES nolli.h DESTINATION include)

add_executable(nolli-exe main.c)
set_target_properties(nolli-exe PROPERTIES OUTPUT_NAME nolli)
target_link_libraries(nolli-exe nolli)
install(TARGETS nolli-exe DESTINATION bin)

set_source_files_properties(tags PROPERTIES GENERATED true)
add_custom_command(
    OUTPUT tags
    COMMAND ctags -R
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
