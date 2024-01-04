cmake_minimum_required(VERSION 3.12)
project(Tutorial VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_executable(Tutorial tutorial.cpp)

add_subdirectory(print_functions) # Add library declaration
target_link_libraries(Tutorial PUBLIC print_functions) # Link the executable against the library (library declared in other CMake, so no need for a path)

# In this example, we declared print_functions in the CMakeLists inside of the print_functions directory. However,
# we can also declare the library in the same CMakeLists as the executable. We'll just need fancy paths to the
# source code.

# Q: Do I need to manually enumerate all the source files in the library?
# Q: How can I reference a library that is not under the PROJECT_SOURCE_DIR?
# Q: How can I define macro on the command line?