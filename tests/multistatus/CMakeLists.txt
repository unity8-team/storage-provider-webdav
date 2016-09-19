add_executable(multistatus_test multistatus_test.cpp)
target_link_libraries(multistatus_test
  owncloud-provider-lib
  Qt5::Test
  gtest
)
add_test(multistatus_test multistatus_test)
