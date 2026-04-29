cmake_minimum_required(VERSION 3.16)

project(mcp-server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

find_package(CURL REQUIRED)

find_package(nlohmann_json REQUIRED)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux/share)
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux/include)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux/lib)

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(httplib REQUIRED)
find_package(SQLiteCpp REQUIRED)

add_subdirectory(src)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()