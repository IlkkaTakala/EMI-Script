cmake_minimum_required(VERSION 3.10)

# set the project name
project(Win11Startup)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(_src_root_path "${Win11Startup_SOURCE_DIR}")
file(
    GLOB_RECURSE _source_list 
    LIST_DIRECTORIES false
    "${_src_root_path}/*.cpp*"
    "${_src_root_path}/*.h*"
)

add_executable(Win11Startup ${_source_list})

add_dependencies(Win11Startup EMIGame)
