cmake_minimum_required(VERSION 3.15)
project(xequation LANGUAGES CXX)

if(ENABLE_GUI_SUPPORT)
    # Network is needed by the RPC layer: QLocalServer / QLocalSocket (the
    # local-socket transport) live there.
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg Network)
    add_library(qt_config INTERFACE)
    target_link_libraries(qt_config INTERFACE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg Qt5::Network)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

enable_testing()

# ---- REL runtime (rel::Value) -------------------------------------------
# xdataset and REL are git submodules (3rd/xdataset, 3rd/REL), but their
# upstream CMakeLists.txt is NOT invoked: upstream is organized as a standalone
# project() and would pollute the exported targets / CTest.  3rd/vendor
# rewrites the build and only creates the targets xequation needs: xdataset,
# rel.
# Python bridge is optional: with BUILD_PYTHON=ON (default) rel compiles the
# embedded CPython bindings (pybind11::embed + python_loader) and env.json's
# python_plugins can load; the host (demo) initializes / shuts down the
# interpreter via python_manager.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/REL/include/value.h")
    message(FATAL_ERROR
        "REL submodule not found at ${CMAKE_CURRENT_SOURCE_DIR}/3rd/REL. "
        "Run 'git submodule update --init' first.")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/xdataset/include/data_frame.h")
    message(FATAL_ERROR
        "xdataset submodule not found at ${CMAKE_CURRENT_SOURCE_DIR}/3rd/xdataset. "
        "Run 'git submodule update --init' first.")
endif()
option(BUILD_PYTHON "Enable embedded Python + plugin support (env.json python_plugins)" ON)
add_subdirectory(3rd/vendor)

add_subdirectory(src)
add_subdirectory(tests)
if(ENABLE_GUI_SUPPORT)
    add_subdirectory(demo)
endif()
