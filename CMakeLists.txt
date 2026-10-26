cmake_minimum_required(VERSION 3.16)
project(cpp_renderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认 Release，但保留调试符号，方便 perf 排查
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 警告全开：早期踩的坑，后期不用还债
if(MSVC)
    # /utf-8：源码是 UTF-8（注释里有中文），不加这个 MSVC 会按系统代码页
    # (CP936) 解码，报 C4819 甚至“常量中有换行符”这类假错误
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(raytracer
    src/main.cpp
)

target_include_directories(raytracer PRIVATE include)

# 每次编译后确保 output 目录存在
add_custom_command(TARGET raytracer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/output
)
