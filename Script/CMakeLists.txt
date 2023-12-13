cmake_minimum_required(VERSION 3.10)

# set the project name
project(ScriptCore)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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

set(Script_INCLUDE_DIRS ${_header_path})
