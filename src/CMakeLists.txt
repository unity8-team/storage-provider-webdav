
add_library(owncloud-provider-lib STATIC
  DavProvider.cpp
  OwncloudProvider.cpp
)
target_compile_options(owncloud-provider-lib PUBLIC
  ${SF_PROVIDER_CFLAGS}
  )
target_include_directories(owncloud-provider-lib PUBLIC
  ${Boost_INCLUDE_DIRS}
)
target_link_libraries(owncloud-provider-lib
  ${Boost_LIBRARIES}
  ${SF_PROVIDER_LDFLAGS}
  Qt5::Network
)

add_executable(storage-provider-owncloud
  main.cpp
)
target_link_libraries(storage-provider-owncloud
  owncloud-provider-lib
)
