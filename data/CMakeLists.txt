# OnlineAccounts support files
install(FILES storage-provider-owncloud.service
  DESTINATION "${CMAKE_INSTALL_DATADIR}/accounts/services")
install(FILES storage-provider-owncloud.application
  DESTINATION "${CMAKE_INSTALL_DATADIR}/accounts/applications")
install(FILES storage-provider-owncloud.desktop
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")

# D-Bus serice activation
configure_file(
  com.canonical.StorageFramework.Provider.OwnCloud.service.in
  com.canonical.StorageFramework.Provider.OwnCloud.service
)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/com.canonical.StorageFramework.Provider.OwnCloud.service
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services
)
