cmake_minimum_required(VERSION 3.10)

# set the project name
project(Game)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(_src_root_path "${Game_SOURCE_DIR}")
file(
    GLOB_RECURSE _source_list 
    LIST_DIRECTORIES false
    "${_src_root_path}/*.cpp*"
    "${_src_root_path}/*.h*"
)

add_executable(EMIGame ${_source_list})

add_custom_command(
    TARGET EMIGame 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${Game_SOURCE_DIR}/../Scripts"
        "${Game_BINARY_DIR}/Scripts"
)

target_link_libraries(EMIGame PUBLIC EMI)
