find_package(GMock)

configure_file(testsetup.h.in testsetup.h @ONLY)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(utils)

set(unit_test_dirs
  multistatus
  item_id
  davprovider
  http_error
  nextcloudprovider
)

set(UNIT_TEST_TARGETS "")
foreach(dir ${unit_test_dirs})
  add_subdirectory(${dir})
  list(APPEND UNIT_TEST_TARGETS "${dir}_test")
endforeach()

set(UNIT_TEST_TARGETS ${UNIT_TEST_TARGETS} PARENT_SCOPE)
