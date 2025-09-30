find_package(GTest REQUIRED)

function(add_gtest_executable target_name test_label source_file)
    set(options "")
    set(oneValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${target_name} ${source_file})


    target_link_libraries(${target_name} PRIVATE
        xexprengine
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
    )

    gtest_discover_tests(${target_name}
        PROPERTIES LABELS ${test_label}
    )

endfunction()

add_gtest_executable(value_test "Value" value_test.cc)
add_gtest_executable(dependency_graph_test "DependencyGraph" dependency_graph_test.cc)
add_gtest_executable(expr_context_test "ExprContext" expr_context_test.cc)
add_gtest_executable(pybind_cast_test "PyObjectConverter" pybind_cast_test.cc)
add_gtest_executable(py_symbol_extractor_test "PySymbolExtractor" py_symbol_extractor_test.cc)