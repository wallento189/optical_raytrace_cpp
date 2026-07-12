cmake_minimum_required(VERSION 3.16)

project(optical_raytrace_cpp LANGUAGES CXX)

# 使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 统一三个平台的可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Visual Studio 是多配置生成器，需要单独设置各配置的输出目录，
# 避免生成到 build/bin/Release/ 下面
foreach(CONFIG_TYPE Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${CONFIG_TYPE}" CONFIG_TYPE_UPPER)
    set(
        CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER}
        "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()

# 自动收集源文件
file(
    GLOB SOURCES
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_executable(optical_raytrace ${SOURCES})

# 目标级头文件目录
target_include_directories(
    optical_raytrace
    PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/third_party"
)

# 根据编译器分别配置编译参数
if(MSVC)
    target_compile_options(
        optical_raytrace
        PRIVATE
            /W4
            /utf-8
            $<$<CONFIG:Release>:/O2>
    )
else()
    target_compile_options(
        optical_raytrace
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:-O2>
    )
endif()