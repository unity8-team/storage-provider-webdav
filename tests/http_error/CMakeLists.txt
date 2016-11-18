add_executable(http_error_test http_error_test.cpp)
target_link_libraries(http_error_test
  dav-provider-lib
  testutils
  Qt5::Test
  gtest
)
add_test(http_error_test http_error_test)
