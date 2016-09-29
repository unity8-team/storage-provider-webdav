
add_library(dav-provider-lib STATIC
  DavProvider.cpp
  RootsHandler.cpp
  MultiStatusParser.cpp
  item_id.cpp
)
target_compile_options(dav-provider-lib PUBLIC
  ${SF_PROVIDER_CFLAGS}
)
target_include_directories(dav-provider-lib PUBLIC
  ${Boost_INCLUDE_DIRS}
)
target_link_libraries(dav-provider-lib
  ${SF_PROVIDER_LDFLAGS}
  ${Boost_LIBRARIES}
  Qt5::Network
  Qt5::Xml
)
set_target_properties(dav-provider-lib PROPERTIES
  AUTOMOC TRUE
)

add_executable(storage-provider-owncloud
  main.cpp
  OwncloudProvider.cpp
)
target_link_libraries(storage-provider-owncloud
  dav-provider-lib
)

install(
  TARGETS storage-provider-owncloud
  RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}"
)
