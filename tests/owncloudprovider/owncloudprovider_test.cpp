/*
 * Copyright (C) 2017 Canonical Ltd.
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

#include "../../src/OwncloudProvider.h"

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QUrl>


namespace provider = unity::storage::provider;


TEST(OwncloudProviderTests, base_url)
{
    provider::PasswordCredentials credentials;
    credentials.username = "username";
    credentials.password = "password";
    credentials.host = "http://example.com/owncloud/";

    provider::Context context;
    context.uid = 0;
    context.pid = 0;
    context.credentials = credentials;

    OwncloudProvider provider;
    auto url = provider.base_url(context).toEncoded().toStdString();
    EXPECT_EQ("http://example.com/owncloud/remote.php/dav/files/username/", url);
}

int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
