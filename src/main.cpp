/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: James Henstridge <james.henstridge@canonical.com>
 */

#include <unity/storage/provider/Server.h>

#include "OwncloudProvider.h"

using namespace std;
using namespace unity::storage::provider;

int main(int argc, char **argv)
{
    string const bus_name = "com.canonical.StorageFramework.Provider.OwnCloud";
    string const account_service_id = "storage-provider-owncloud";

    Server<OwncloudProvider> server(bus_name, account_service_id);
    server.init(argc, argv);
    server.run();
    return 0;
}
