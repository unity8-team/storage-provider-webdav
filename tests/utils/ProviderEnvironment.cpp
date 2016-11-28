/*
 * Copyright (C) 2016 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: James Henstridge <james.henstridge@canonical.com>
 */

#include "ProviderEnvironment.h"

#include <unity/storage/provider/ProviderBase.h>

#include <cassert>

using namespace std;
using namespace unity::storage::provider;
using namespace unity::storage::qt;

namespace
{

const auto SERVICE_CONNECTION_NAME = QStringLiteral("service-session-bus");
const auto OBJECT_PATH = QStringLiteral("/provider");

}

ProviderEnvironment::ProviderEnvironment(shared_ptr<ProviderBase> const& provider,
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
    server_.reset(new testing::TestServer(provider, account,
                                          *server_connection_,
                                          OBJECT_PATH.toStdString()));

    client_runtime_.reset(new Runtime(*client_connection_));
    client_account_ = client_runtime_->make_test_account(
        server_connection_->baseService(), OBJECT_PATH);
}

ProviderEnvironment::~ProviderEnvironment()
{
    client_runtime_->shutdown();
    client_runtime_.reset();

    server_.reset();
    account_manager_.reset();

    server_connection_.reset();
    QDBusConnection::disconnectFromBus(SERVICE_CONNECTION_NAME);
    client_connection_.reset();
}

Account ProviderEnvironment::get_client() const
{
    return client_account_;
}
