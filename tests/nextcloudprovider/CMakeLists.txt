add_executable(nextcloudprovider_test nextcloudprovider_test.cpp)
target_link_libraries(nextcloudprovider_test
  dav-provider-lib
  Qt5::Test
  gtest
)
add_test(nextcloudprovider_test nextcloudprovider_test)
