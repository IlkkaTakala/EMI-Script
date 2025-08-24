cmake_minimum_required(VERSION 3.10)

# set the project name
project(AStarFinder)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(_src_root_path "${AStarFinder_SOURCE_DIR}")
file(
    GLOB_RECURSE _source_list 
    LIST_DIRECTORIES false
    "${_src_root_path}/*.cpp*"
    "${_src_root_path}/*.h*"
)

add_executable(AStarFinder ${_source_list})

add_custom_command(
    TARGET AStarFinder 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${AStarFinder_SOURCE_DIR}/../Scripts"
        "${AStarFinder_BINARY_DIR}/Scripts"
)

target_link_libraries(AStarFinder PUBLIC EMI)
