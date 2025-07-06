cmake_minimum_required(VERSION 3.10)

project(Template LANGUAGES CXX)

find_package(Python REQUIRED COMPONENTS Development Interpreter)

message(STATUS "Python executable: ${Python_EXECUTABLE}")
message(STATUS "Python include dirs: ${Python_INCLUDE_DIRS}")
message(STATUS "Python libraries: ${Python_LIBRARIES}")
find_package(pybind11 REQUIRED)
find_package(fmt REQUIRED)

set(PYTHON_VENV_DIR ${CMAKE_CURRENT_BINARY_DIR}/venv)
set(VENV_CONFIG ${PYTHON_VENV_DIR}/pyvenv.cfg)
set(REQUIREMENTS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt)
set(DEPS_INSTALLED_MARKER "${VENV_DIR}/.deps_installed")


if(NOT EXISTS ${REQUIREMENTS_FILE})
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/requirements.tmp "# Python dependencies\n")
    file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/requirements.tmp ${REQUIREMENTS_FILE})
endif()

add_custom_command(
    OUTPUT ${VENV_CONFIG}
    COMMAND "${CMAKE_COMMAND}" -E make_directory ${PYTHON_VENV_DIR}
    COMMAND ${Python_EXECUTABLE} -m venv ${PYTHON_VENV_DIR}
    COMMENT "Setting up Python virtual environment"
    DEPENDS ${Python_EXECUTABLE}
    VERBATIM
)

add_custom_command(
    OUTPUT ${DEPS_INSTALLED_MARKER}
    COMMAND ${PYTHON_VENV_DIR}/Scripts/pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    COMMAND "${CMAKE_COMMAND}" -E touch "${DEPS_INSTALLED_MARKER}"
    DEPENDS ${VENV_CONFIG} ${REQUIREMENTS_FILE}
    COMMENT "Installing Python dependencies"
    VERBATIM
)

add_custom_target(venv ALL DEPENDS ${VENV_CONFIG} ${DEPS_INSTALLED_MARKER})

add_executable(Template main.cc)
add_dependencies(Template venv)

target_link_libraries(Template PRIVATE pybind11::embed)