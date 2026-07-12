cmake_minimum_required(VERSION 3.16)
project(optical_raytrace_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/third_party)

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(optical_raytrace ${SOURCES})

target_compile_options(optical_raytrace PRIVATE
    -Wall -Wextra -O2
)

if(MSVC)
    target_compile_options(optical_raytrace PRIVATE /W4 /O2)
endif()
