add_library(print_functions STATIC goodbye_world.cpp hello_world.cpp)

# Any executables linking to this library get this include directory (for free!)
target_include_directories(print_functions INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}) 
