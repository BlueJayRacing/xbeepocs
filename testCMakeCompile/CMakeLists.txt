cmake_minimum_required(VERSION 3.12)
project(MyProject VERSION 1.0.0)

add_library(
    xbee STATIC
    # What do I place here? He used a header and source file,
    # but I have lots of headers and source files.
)


add_executable(MyExecutable my_transmitter.cpp)

target_link_libraries(MyExecutable PRIVATE xbee-include)

# Within the build directory
# cmake ..
# cmake --build .


# We only need to run cmake once, and then we can run "cmake --build ." multiple times