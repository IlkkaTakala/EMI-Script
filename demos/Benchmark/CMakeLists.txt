cmake_minimum_required(VERSION 3.10)

# set the project name
project(Benchmarks)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(_src_root_path "${Benchmarks_SOURCE_DIR}")

add_executable(Benchmarks main.cpp)

add_custom_command(
    TARGET Benchmarks 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${Benchmarks_SOURCE_DIR}/Scripts"
        "${Benchmarks_BINARY_DIR}/Scripts"
)

target_link_libraries(Benchmarks PUBLIC EMI)
