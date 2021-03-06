cmake_minimum_required(VERSION 3.1)

enable_testing()

set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

INCLUDE_DIRECTORIES(../ext/ ../src/)
LINK_DIRECTORIES(../cmake)

find_package(Threads)

# Build Catch unit tests
add_library(catch INTERFACE)
target_include_directories(catch INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


if (${APPLE})
    MESSAGE("Host platform: MAC OS X")
elseif (${UNIX})
    MESSAGE("Host platform:  Other UNIX")
    set(WG_LIBRARIES "pthread uuid rt")
endif ()

MESSAGE("Using libraries : [" ${WG_LIBRARIES} "]")

add_executable(test_config common/test_config.cpp)
target_link_libraries(test_config watergate ${CMAKE_THREAD_LIBS_INIT} ${WG_LIBRARIES})
add_test(NAME test_config COMMAND test_config)

add_executable(test_sem_lock core/test_sem_lock.cpp)
target_link_libraries(test_sem_lock watergate ${CMAKE_THREAD_LIBS_INIT} ${WG_LIBRARIES})
add_test(NAME test_sem_lock COMMAND test_sem_lock)

add_executable(test_alarm common/test_alarm.cpp)
target_link_libraries(test_alarm watergate ${CMAKE_THREAD_LIBS_INIT} ${WG_LIBRARIES})
add_test(NAME test_alarm COMMAND test_alarm)

add_executable(test_lock_client core/test_lock_client.cpp)
target_link_libraries(test_lock_client watergate ${CMAKE_THREAD_LIBS_INIT} ${WG_LIBRARIES})

add_executable(test_lock_table core/test_lock_table.cpp)
target_link_libraries(test_lock_table watergate ${CMAKE_THREAD_LIBS_INIT} ${WG_LIBRARIES})
add_test(NAME test_lock_table COMMAND test_lock_table)

add_executable(test_fs_writer core/io/test_fs_writer.cpp)
target_link_libraries(test_fs_writer watergate ${CMAKE_THREAD_LIBS_INIT} ${WG_LIBRARIES})
add_test(NAME test_fs_writer COMMAND test_fs_writer)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/logs")
