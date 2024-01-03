cmake_minimum_required(VERSION 3.10)

# set the project name
project(ScriptCore)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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

set(_src_root_path "${ScriptCore_SOURCE_DIR}/src")
set(_header_path "${ScriptCore_SOURCE_DIR}/include")
file(
    GLOB_RECURSE _source_list 
    LIST_DIRECTORIES false
    "${_src_root_path}/*.cpp*"
    "${_src_root_path}/*.h*"
    "${_header_path}/*.h*"
)

add_library(Script ${_source_list})

target_compile_definitions(Script PRIVATE SCRIPTEXPORT)
target_include_directories(Script PUBLIC ${_header_path})
target_include_directories(Script INTERFACE ${_header_path})

target_link_libraries(Script PUBLIC unordered_dense)


set(Script_INCLUDE_DIRS ${_header_path})
