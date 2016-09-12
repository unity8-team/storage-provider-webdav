
add_library(owncloud-provider-lib STATIC
  DavProvider.cpp
  )
target_compile_options(owncloud-provider-lib PUBLIC
  ${SF_PROVIDER_CFLAGS})
target_link_libraries(owncloud-provider-lib
  ${SF_PROVIDER_LDFLAGS})
