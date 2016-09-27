add_executable(item_id_test item_id_test.cpp)
target_link_libraries(item_id_test
  dav-provider-lib
  Qt5::Test
  gtest
)
add_test(item_id_test item_id_test)
