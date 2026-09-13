add_executable(test_suite test_suite.cpp)
target_link_libraries(test_suite PRIVATE lambda_brief_core)

add_test(NAME test_suite COMMAND test_suite)
