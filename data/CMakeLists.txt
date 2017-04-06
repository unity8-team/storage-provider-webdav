# OnlineAccounts support files
install(
  FILES
    accounts/storage-provider-nextcloud.service
    accounts/storage-provider-owncloud.service
  DESTINATION "${CMAKE_INSTALL_DATADIR}/accounts/services")
install(
  FILES
    accounts/storage-provider-nextcloud.application
    accounts/storage-provider-owncloud.application
  DESTINATION "${CMAKE_INSTALL_DATADIR}/accounts/applications")
install(
  FILES
    accounts/storage-provider-nextcloud.desktop
    accounts/storage-provider-owncloud.desktop
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")

# D-Bus serice activation
configure_file(
  dbus/com.canonical.StorageFramework.Provider.Nextcloud.service.in
  dbus/com.canonical.StorageFramework.Provider.Nextcloud.service
)
configure_file(
  dbus/com.canonical.StorageFramework.Provider.OwnCloud.service.in
  dbus/com.canonical.StorageFramework.Provider.OwnCloud.service
)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dbus/com.canonical.StorageFramework.Provider.Nextcloud.service
    ${CMAKE_CURRENT_BINARY_DIR}/dbus/com.canonical.StorageFramework.Provider.OwnCloud.service
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services
)

# Systemd unit files
configure_file(
  systemd/storage-provider-nextcloud.service.in
  systemd/storage-provider-nextcloud.service
)
configure_file(
  systemd/storage-provider-owncloud.service.in
  systemd/storage-provider-owncloud.service
)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/systemd/storage-provider-nextcloud.service
    ${CMAKE_CURRENT_BINARY_DIR}/systemd/storage-provider-owncloud.service
  DESTINATION "lib/systemd/user"
)
