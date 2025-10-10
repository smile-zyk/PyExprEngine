cmake_minimum_required(VERSION 3.15)
project(xexprengine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(Python_ROOT_DIR "${CMAKE_SOURCE_DIR}/python-3.8.8rc1")
set(pybind11_DIR "${Python_ROOT_DIR}/Lib/site-packages/pybind11/share/cmake/pybind11")

enable_testing()
add_subdirectory(xexprengine)
add_subdirectory(tests)
