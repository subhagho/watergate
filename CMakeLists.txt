cmake_minimum_required(VERSION 3.1)


project(watergate VERSION 0.1.0)


set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

INCLUDE_DIRECTORIES(ext/ src/)

set(WATERGATE_LIB_NAME ${PROJECT_NAME})

file(GLOB_RECURSE WG_SOURCES "src/*.cpp")
file(GLOB_RECURSE WG_INCLUDES "src/*.h")
file(GLOB_RECURSE WG_EXT_INCLUDES "src/includes/*.h")
list(REMOVE_ITEM WG_SOURCES "src/test/main.cpp")

add_library(${WATERGATE_LIB_NAME} SHARED ${WG_INCLUDES} ${WG_SOURCES})

add_executable(main src/test/main.cpp)
target_link_libraries(main watergate)