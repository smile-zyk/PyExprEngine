find_package(GTest CONFIG REQUIRED)
add_executable(unit_tests 
    exprvalue_test.cc
)

target_link_libraries(unit_tests PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
target_link_libraries(unit_tests PRIVATE xexprengine)

# 添加测试
enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)