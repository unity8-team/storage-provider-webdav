
add_library(dav-provider-lib STATIC
  DavProvider.cpp
  DavDownloadJob.cpp
  DavUploadJob.cpp
  MultiStatusParser.cpp
  http_error.cpp
  item_id.cpp
  CopyMoveHandler.cpp
  CreateFolderHandler.cpp
  DeleteHandler.cpp
  PropFindHandler.cpp
  ListHandler.cpp
  LookupHandler.cpp
  MetadataHandler.cpp
  RetrieveMetadataHandler.cpp
  RootsHandler.cpp
  NextcloudProvider.cpp
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

add_executable(storage-provider-nextcloud
  nextcloud.cpp
)
target_link_libraries(storage-provider-nextcloud
  dav-provider-lib
)

add_executable(storage-provider-owncloud
  owncloud.cpp
)
target_link_libraries(storage-provider-owncloud
  dav-provider-lib
)

install(
  TARGETS
    storage-provider-nextcloud
    storage-provider-owncloud
  RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}"
)
