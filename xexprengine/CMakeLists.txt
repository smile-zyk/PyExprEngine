find_package(Python REQUIRED COMPONENTS Development Interpreter)

message(STATUS "Python executable: ${Python_EXECUTABLE}")
message(STATUS "Python include dirs: ${Python_INCLUDE_DIRS}")
message(STATUS "Python libraries: ${Python_LIBRARIES}")
find_package(pybind11 REQUIRED)

aux_source_directory(. XEXPRENGINE_SRC)

add_library(xexprengine SHARED ${XEXPRENGINE_SRC})

target_include_directories(xexprengine PUBLIC .)
target_link_libraries(xexprengine PRIVATE pybind11::embed)