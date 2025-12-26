find_package(Boost REQUIRED COMPONENTS multi_index uuid compute)
find_path(TSL_ORDERED_MAP_INCLUDE_DIRS "tsl/ordered_hash.h")

set(BIN2C_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/python/bin2c.py)

set(PYTHON_PARSER_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/python/python_parser.py)
set(PYTHON_PARSER_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/python)
set(PYTHON_PARSER_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/python/python_parser_embed.h)

set(xequation_SRC
    ${PYTHON_PARSER_OUTPUT}
    core/value.h
    core/value.cc
    core/dependency_graph.h
    core/dependency_graph.cc
    core/equation.h
    core/equation.cc
    core/equation_group.h
    core/equation_group.cc
    core/equation_manager.h
    core/equation_manager.cc
    core/equation_engine.h
    core/equation_context.h
    core/equation_common.h
    core/equation_signals_manager.h
    python/python_base.h
    python/python_executor.h
    python/python_executor.cc
    python/python_parser.h
    python/python_parser.cc
    python/value_pybind_converter.h
    python/python_equation_context.h
    python/python_equation_context.cc
    python/python_equation_engine.h
    python/python_equation_engine.cc
)

add_library(xequation STATIC ${xequation_SRC})

add_custom_target(generate_python_parser ALL
    COMMAND ${Python_EXECUTABLE} ${BIN2C_SCRIPT} ${PYTHON_PARSER_INPUT} 
            -o ${PYTHON_PARSER_OUTPUT_DIR} -v python_parser -n xequation
    DEPENDS ${BIN2C_SCRIPT} ${PYTHON_PARSER_INPUT}
    BYPRODUCTS ${PYTHON_PARSER_OUTPUT}
    COMMENT "Generating python_parser_embed.h from python_parser.py"
    VERBATIM
)

add_dependencies(xequation generate_python_parser)

set_target_properties(xequation PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(xequation PUBLIC pybind11::headers)
target_link_libraries(xequation PUBLIC Python::Python)
target_link_libraries(xequation PUBLIC Boost::multi_index)
target_link_libraries(xequation PUBLIC Boost::uuid)
target_link_libraries(xequation PUBLIC Boost::compute)

target_include_directories(xequation PUBLIC ./)
target_include_directories(xequation PUBLIC ${TSL_ORDERED_MAP_INCLUDE_DIRS})