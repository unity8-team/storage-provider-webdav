pkg_check_modules(TESTUTILS_DEPS libqtdbustest-1 OnlineAccountsQt REQUIRED)

add_library(testutils STATIC
  DBusEnvironment.cpp
  DavEnvironment.cpp
  ProviderEnvironment.cpp
)
target_compile_options(testutils
  PUBLIC ${SF_PROVIDER_CFLAGS} ${SF_CLIENT_CFLAGS} ${TESTUTILS_DEPS_CFLAGS})
target_link_libraries(testutils
  PUBLIC Qt5::DBus Qt5::Core ${SF_PROVIDER_LDFLAGS} ${SF_CLIENT_LDFLAGS}
  PRIVATE ${TESTUTILS_DEPS_LDFLAGS})
