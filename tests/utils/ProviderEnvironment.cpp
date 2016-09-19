#include "ProviderEnvironment.h"

#include <cassert>

using namespace std;
using namespace unity::storage::provider;
using namespace unity::storage::qt::client;

namespace
{

const auto SERVICE_CONNECTION_NAME = QStringLiteral("service-session-bus");
const auto OBJECT_PATH = QStringLiteral("/provider");

}

ProviderEnvironment::ProviderEnvironment(unique_ptr<ProviderBase>&& provider,
                                         OnlineAccounts::AccountId account_id,
                                         DBusEnvironment const& dbus_env)
{
    client_connection_.reset(new QDBusConnection(dbus_env.connection()));
    server_connection_.reset(new QDBusConnection(
        QDBusConnection::connectToBus(dbus_env.busAddress(),
                                      SERVICE_CONNECTION_NAME)));

    account_manager_.reset(new OnlineAccounts::Manager("", *server_connection_));
    account_manager_->waitForReady();
    OnlineAccounts::Account* account = account_manager_->account(account_id);
    assert(account != nullptr);
    server_.reset(new testing::TestServer(move(provider), account,
                                          *server_connection_,
                                          OBJECT_PATH.toStdString()));

    client_runtime_ = Runtime::create(*client_connection_);
    client_account_ = client_runtime_->make_test_account(server_connection_->baseService(), OBJECT_PATH);
}

ProviderEnvironment::~ProviderEnvironment()
{
    client_account_.reset();
    client_runtime_->shutdown();
    client_runtime_.reset();

    server_.reset();
    account_manager_.reset();

    server_connection_.reset();
    QDBusConnection::disconnectFromBus(SERVICE_CONNECTION_NAME);
    client_connection_.reset();
}

shared_ptr<Account> ProviderEnvironment::get_client() const
{
    return client_account_;
}
