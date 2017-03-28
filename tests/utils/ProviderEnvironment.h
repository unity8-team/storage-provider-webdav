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

#pragma once

#include "DBusEnvironment.h"

#include <QDBusConnection>
#include <unity/storage/provider/testing/TestServer.h>
#include <unity/storage/qt/Account.h>
#include <unity/storage/qt/Runtime.h>

#include <memory>

class ProviderEnvironment
{
public:
    ProviderEnvironment(std::shared_ptr<unity::storage::provider::ProviderBase> const& provider,
                        DBusEnvironment const& dbus_env);
    ~ProviderEnvironment();

    unity::storage::qt::Account get_client() const;

private:
    std::unique_ptr<QDBusConnection> client_connection_;
    std::unique_ptr<QDBusConnection> server_connection_;
    std::unique_ptr<unity::storage::provider::testing::TestServer> server_;

    std::unique_ptr<unity::storage::qt::Runtime> client_runtime_;
    unity::storage::qt::Account client_account_;
};
