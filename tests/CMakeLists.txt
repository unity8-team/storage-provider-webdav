# We add -g so we get debug info for the gtest stack frames with gdb.
# The warnings are suppressed so we get a noise-free build for gtest and gmock.

set(old_cxx_flags ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wno-old-style-cast -Wno-missing-field-initializers")
find_package(GMock)
set(CMAKE_CXX_FLAGS ${old_cxx_flags})

configure_file(testsetup.h.in testsetup.h @ONLY)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(utils)

set(unit_test_dirs
  multistatus
  item_id
)

set(UNIT_TEST_TARGETS "")
foreach(dir ${unit_test_dirs})
  add_subdirectory(${dir})
  list(APPEND UNIT_TEST_TARGETS "${dir}_test")
endforeach()

set(UNIT_TEST_TARGETS ${UNIT_TEST_TARGETS} PARENT_SCOPE)
