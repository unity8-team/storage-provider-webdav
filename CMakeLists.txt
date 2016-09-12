cmake_minimum_required(VERSION 3.0.2)
project(storage-provider-owncloud VERSION "0.1")

include(CTest)
include(EnableCoverageReport)
include(FindPkgConfig)
include(GNUInstallDirs)

enable_testing()

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
pkg_check_modules(SF_PROVIDER REQUIRED storage-framework-provider-1)

add_definitions(-DQT_NO_KEYWORDS)

add_subdirectory(src)
