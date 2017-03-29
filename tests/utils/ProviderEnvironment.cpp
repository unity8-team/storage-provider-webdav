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

#include <libqtdbustest/DBusTestRunner.h>
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

ProviderEnvironment::ProviderEnvironment(shared_ptr<ProviderBase> const& provider)
{
    runner_.reset(new QtDBusTest::DBusTestRunner());
    client_connection_.reset(new QDBusConnection(runner_->sessionConnection()));
    server_connection_.reset(new QDBusConnection(
        QDBusConnection::connectToBus(runner_->sessionBus(),
                                      SERVICE_CONNECTION_NAME)));

    server_.reset(new testing::TestServer(provider, nullptr,
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

    server_connection_.reset();
    QDBusConnection::disconnectFromBus(SERVICE_CONNECTION_NAME);
    client_connection_.reset();
    runner_.reset();
}

Account ProviderEnvironment::get_client() const
{
    return client_account_;
}
