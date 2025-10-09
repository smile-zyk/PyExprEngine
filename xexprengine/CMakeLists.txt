find_package(Python REQUIRED COMPONENTS Development Interpreter)
find_package(pybind11 REQUIRED)

find_package(Boost 1.79.0 REQUIRED COMPONENTS serialization)

message(STATUS "Python executable: ${Python_EXECUTABLE}")
message(STATUS "Python include dirs: ${Python_INCLUDE_DIRS}")
message(STATUS "Python libraries: ${Python_LIBRARIES}")

set(XEXPRENGINE_SRC
    core/value.cc
    core/dependency_graph.cc
    core/variable.cc
    core/event_stamp.cc
    core/variable_manager.cc
    python/value_pybind_converter.cc
    python/py_symbol_extractor.cc
    python/py_expr_context.cc
    python/py_expr_engine.cc
)

add_library(xexprengine STATIC ${XEXPRENGINE_SRC} )

set_target_properties(xexprengine PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_include_directories(xexprengine PUBLIC . ${Python_INCLUDE_DIRS})
target_link_libraries(xexprengine PRIVATE pybind11::embed ${Python_LIBRARIES})
target_link_libraries(xexprengine PRIVATE Boost::serialization)

target_include_directories(xexprengine PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(xexprengine PUBLIC ./)