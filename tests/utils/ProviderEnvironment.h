#pragma once

#include "DBusEnvironment.h"

#include <OnlineAccounts/Manager>
#include <QDBusConnection>
#include <unity/storage/provider/testing/TestServer.h>
#include <unity/storage/qt/client/Runtime.h>

#include <memory>

class ProviderEnvironment
{
public:
    ProviderEnvironment(std::unique_ptr<unity::storage::provider::ProviderBase>&& provider,
                        OnlineAccounts::AccountId account_id,
                        DBusEnvironment const& dbus_env);
    ~ProviderEnvironment();

    std::shared_ptr<unity::storage::qt::client::Account> get_client() const;

private:
    std::unique_ptr<QDBusConnection> client_connection_;
    std::unique_ptr<QDBusConnection> server_connection_;
    std::unique_ptr<OnlineAccounts::Manager> account_manager_;
    std::unique_ptr<unity::storage::provider::testing::TestServer> server_;
    std::shared_ptr<unity::storage::qt::client::Runtime> client_runtime_;
    std::shared_ptr<unity::storage::qt::client::Account> client_account_;
};
