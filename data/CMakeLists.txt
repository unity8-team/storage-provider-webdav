install(FILES storage-provider-owncloud.service
  DESTINATION "${CMAKE_INSTALL_DATADIR}/accounts/services")

install(FILES storage-provider-owncloud.application
  DESTINATION "${CMAKE_INSTALL_DATADIR}/accounts/applications")

install(FILES storage-provider-owncloud.desktop
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
