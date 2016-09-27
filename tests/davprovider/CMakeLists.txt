add_executable(davprovider_test davprovider_test.cpp)
target_link_libraries(davprovider_test
  dav-provider-lib
  testutils
  Qt5::Test
  gtest
)
add_test(davprovider_test davprovider_test)
