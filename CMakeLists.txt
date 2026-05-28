cmake_minimum_required(VERSION 3.28.3)
project(rpc)

# 设置项目可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 设置项目库文件输出的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置项目编译头文件的搜索路径 -I
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/example)

# 设置项目库文件的搜索路径 -L
link_directories(${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(src)
add_subdirectory(example)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Release CXX flags: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "Debug CXX flags: ${CMAKE_CXX_FLAGS_DEBUG}")