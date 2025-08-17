find_package(GTest REQUIRED)

add_executable(unit_tests exprvalue_test.cc)

target_link_libraries(unit_tests PRIVATE
    xexprengine
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
)

target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/xexprengine)
include(CTest)

add_test(
    NAME unit_tests
    COMMAND unit_tests
)
