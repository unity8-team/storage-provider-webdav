cmake_minimum_required(VERSION 3.0.2)
project(storage-provider-webdav VERSION "0.1")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic -Wextra -fvisibility=hidden")

include(CTest)
include(EnableCoverageReport)
include(FindPkgConfig)
include(GNUInstallDirs)

enable_testing()

find_package(Boost 1.56 COMPONENTS thread system REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Test REQUIRED)
find_package(Qt5Xml REQUIRED)
pkg_check_modules(SF_PROVIDER REQUIRED storage-framework-provider-1)
pkg_check_modules(SF_CLIENT REQUIRED storage-framework-qt-client-1)

add_definitions(-DQT_NO_KEYWORDS)

add_subdirectory(src)
add_subdirectory(tests)

enable_coverage_report(
  TARGETS
    dav-provider-lib
    storage-provider-owncloud
  FILTER
    ${CMAKE_SOURCE_DIR}/tests/*
    ${CMAKE_BINARY_DIR}/*
  TESTS
  ${UNIT_TEST_TARGETS}
)
