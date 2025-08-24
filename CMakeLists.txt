cmake_minimum_required(VERSION 3.10)

# set the project name
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(EMIScript)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
option(EMI_PARSE_GRAMMAR ON)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_COMMAND_LINE "Build command line" ON)
option(BUILD_DEMOS "Include demos in the build" OFF)

include(FetchContent)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -fms-extensions)
endif()

macro(add_dependency)
    SET(dependency_name ${ARGV0})
    SET(endpoint_url ${ARGV1})
    SET(endpoint_tag ${ARGV2})
    SET(do_build_with_cmake ${ARGV3})

    FetchContent_Declare(
            ${dependency_name}
            GIT_REPOSITORY ${endpoint_url}
            GIT_TAG ${endpoint_tag}
    )

    FetchContent_GetProperties(${dependency_name})

    if (NOT ${dependency_name}_POPULATED)
        FetchContent_Populate(${dependency_name})
        message(STATUS "Working on ${dependency_name}")

        if (${do_build_with_cmake})
            add_subdirectory(${${dependency_name}_SOURCE_DIR} ${${dependency_name}_BINARY_DIR})
        else ()
            message("\tHeader only")
        endif ()
    endif ()
endmacro()

add_dependency(unordered_dense https://github.com/martinus/unordered_dense v4.4.0 TRUE)

set(_src_root_path "${EMIScript_SOURCE_DIR}/src")
set(_grammar_path "${EMIScript_SOURCE_DIR}/src/Parser")
set(_header_path "${EMIScript_SOURCE_DIR}/include")
file(
    GLOB_RECURSE _source_list 
    LIST_DIRECTORIES false
    "${_src_root_path}/*.cpp*"
    "${_src_root_path}/*.h*"
    "${_header_path}/*.h*"
)
if (NOT EMI_PARSE_GRAMMAR) 
list(REMOVE_ITEM _source_list "${EMIScript_SOURCE_DIR}/src/Parser/ParseTable.cpp" "${EMIScript_SOURCE_DIR}/src/Parser/ParseTable.h")
endif()

foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${_src_root_path}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

if(BUILD_SHARED_LIBS)
	add_library(EMI SHARED ${_source_list})
	target_compile_definitions(EMI PRIVATE BUILD_SHARED_LIBS)
else()
	add_library(EMI STATIC ${_source_list})
endif()

if (EMI_PARSE_GRAMMAR) 
    target_compile_definitions(EMI PRIVATE EMI_PARSE_GRAMMAR "GRAMMAR_PATH=\"${_grammar_path}\"")
endif()
target_compile_definitions(EMI PRIVATE SCRIPTEXPORT)
target_include_directories(EMI PUBLIC ${_header_path})
target_include_directories(EMI PRIVATE ${_src_root_path})
target_include_directories(EMI INTERFACE ${_header_path})

find_package (Threads)
target_link_libraries (EMI PRIVATE ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(EMI PUBLIC unordered_dense)

set(EMI_INCLUDE_DIRS ${_header_path})

if(BUILD_COMMAND_LINE)
add_subdirectory(demos/CLI)
endif()

if(BUILD_DEMOS)
add_subdirectory(demos/AStarFinder)
set_target_properties(AStarFinder PROPERTIES FOLDER "Demos")
endif()
