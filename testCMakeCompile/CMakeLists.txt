cmake_minimum_required(VERSION 3.12)
project(Transmitter VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(my_transmitter my_transmitter.cpp)

# This top-level CMake needs targets declared in the library's CMake, so
# we use a path to the library's directory
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/xbee_ansic_library
    ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/xbee_ansic_library/bin
    )

# By linking the xbee_library CMake target against the my_transmitter target,
# the compiler knows our executable uses code from the library declared in another CMake
target_link_libraries(my_transmitter PUBLIC xbee_library) 

# Q: How can I reference a library that is not under the PROJECT_SOURCE_DIR?