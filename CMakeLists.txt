cmake_minimum_required(VERSION 3.0)

project(tcerver)

# Add color support to our messages.
# See: https://stackoverflow.com/questions/18968979/how-to-get-colorized-output-with-cmake
if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColorReset   "${Esc}[m")
  set(ColorBold    "${Esc}[1m")
  set(ColorRed     "${Esc}[31m")
  set(ColorGreen   "${Esc}[32m")
  set(ColorYellow  "${Esc}[33m")
  set(ColorBlue    "${Esc}[34m")
  set(ColorMagenta "${Esc}[35m")
  set(ColorCyan    "${Esc}[36m")
  set(ColorWhite   "${Esc}[37m")
endif()

# set entry points
set(entries "server" "queue_test" "util_test")
set(release_entries "server")
# Disable in-source builds (running "cmake ." in the project directory)
# This is bad practice as it pollutes the project directory.
# See: https://dpiepgrass.medium.com/cmake-fights-against-disabling-in-source-builds-ab1d71c1d26f
if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}") 
  message(FATAL_ERROR "${ColorRed}In-source builds are disabled.\n"
    "Create a subdirectory `build/` and use `cmake ..` inside it.\n"
    "${ColorBold}Delete `CMakeCache.txt` and `CMakeFiles/` before you continue.${ColorReset}")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Specify C++ compiler and linker.
# if(NOT DEFINED EWS)
#     set(CMAKE_C_COMPILER "/usr/bin/clang")
#     set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
#     set(CMAKE_LINKER "/usr/bin/clang++")
# else()
#     set(CMAKE_C_COMPILER "/software/llvm-6.0.1/bin/clang")
#     set(CMAKE_CXX_COMPILER "/software/llvm-6.0.1/bin/clang++")
#     set(CMAKE_LINKER "/software/llvm-6.0.1/bin/clang++")

#     # Add other required flags for EWS.

# endif()
set(CMAKE_CXX_COMPILER "/usr/bin/g++-10")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fcoroutines")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -std=c++2a")
 # pthread.h
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

# Specify C++ Standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Specify warnings for all builds.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wfatal-errors -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function ")

# Path definitions.
# set(lib_dir ${CMAKE_SOURCE_DIR}/lib)
set(src_dir ${CMAKE_SOURCE_DIR}/src)
set(tests_dir ${CMAKE_SOURCE_DIR}/tests)
set(entry_dir ${CMAKE_SOURCE_DIR}/entry)


# Run CMakeLists in lib_dir to build our required libraries.
# add_subdirectory(${lib_dir})

# Run CMakeLists in src_dir to build the stude  nt's code.
add_subdirectory(${src_dir})

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)


# Add the assignment entrypoints using our libs and src code.
foreach(entrypoint IN LISTS entries)
    add_executable(${entrypoint} ${entry_dir}/${entrypoint}.cpp)
    target_link_libraries(${entrypoint} PRIVATE src)
    target_compile_options(${entrypoint} PUBLIC -g)
    target_compile_definitions(${entrypoint} PUBLIC DEBUG=1)
endforeach()

# release build
foreach(entrypoint IN LISTS release_entries)
    add_executable(${entrypoint}-release ${entry_dir}/${entrypoint}.cpp)
    target_link_libraries(${entrypoint}-release PRIVATE src)
    target_compile_options(${entrypoint}-release PRIVATE -O3)
endforeach()

# testcases
add_executable("test" ${tests_dir}/test.cpp)
target_link_libraries("test" PRIVATE src)

# creating the static library
ADD_LIBRARY(tcerver STATIC
             src/EPoll.cpp src/HTTPResponse.cpp src/HTTPUnit.cpp src/TCPServer.cpp src/MultiThreadQueue.hpp src/utilities.cpp)
set_target_properties(tcerver PROPERTIES LINKER_LANGUAGE CXX)