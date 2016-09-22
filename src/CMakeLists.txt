
add_library(owncloud-provider-lib STATIC
  DavProvider.cpp
  OwncloudProvider.cpp
  RootsHandler.cpp
  MultiStatusParser.cpp
  item_id.cpp
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
  Qt5::Xml
)
set_target_properties(owncloud-provider-lib PROPERTIES
  AUTOMOC TRUE
)

add_executable(storage-provider-owncloud
  main.cpp
)
target_link_libraries(storage-provider-owncloud
  owncloud-provider-lib
)
