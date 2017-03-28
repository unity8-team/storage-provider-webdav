add_executable(owncloudprovider_test owncloudprovider_test.cpp)
target_link_libraries(owncloudprovider_test
  dav-provider-lib
  Qt5::Test
  gtest
)
add_test(owncloudprovider_test owncloudprovider_test)
