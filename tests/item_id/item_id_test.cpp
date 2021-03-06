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

#include "../../src/item_id.h"

#include <unity/storage/provider/Exceptions.h>
#include <gtest/gtest.h>

using namespace std;
using namespace unity::storage::provider;

TEST(ItemId, id_to_url)
{
    QUrl const base_url("http://example.com/base/dir/");
    auto to_url = [&](string const& item_id) -> string
    {
        QUrl url = id_to_url(item_id, base_url);
        return url.toEncoded().toStdString();
    };

    EXPECT_EQ("http://example.com/base/dir/", to_url("."));
    EXPECT_EQ("http://example.com/base/dir/file", to_url("file"));
    EXPECT_EQ("http://example.com/base/dir/folder/file", to_url("folder/file"));

    EXPECT_EQ("http://example.com/base/dir/folder/file", to_url("folder/./file"));
    EXPECT_EQ("http://example.com/base/dir/file", to_url("folder/../file"));
    EXPECT_EQ("http://example.com/base/dir/hello%20world", to_url("hello%20world"));

    EXPECT_THROW(to_url("::"), InvalidArgumentException);
    EXPECT_THROW(to_url(" "), InvalidArgumentException);
    EXPECT_THROW(to_url(".."), InvalidArgumentException);
    EXPECT_THROW(to_url("folder/../../file"), InvalidArgumentException);
}

TEST(ItemId, url_to_id)
{
    QUrl const base_url("http://example.com/base/dir/");
    auto to_id = [&](string const& item_url) -> string
    {
        QUrl url(QString::fromStdString(item_url), QUrl::StrictMode);
        return url_to_id(url, base_url);
    };

    EXPECT_EQ(".", to_id("http://example.com/base/dir/"));
    EXPECT_EQ("file", to_id("http://example.com/base/dir/file"));
    EXPECT_EQ("folder/file", to_id("http://example.com/base/dir/folder/file"));

    EXPECT_EQ("folder/file", to_id("http://example.com/base/dir/folder/./file"));
    EXPECT_EQ("file", to_id("http://example.com/base/dir/folder/../file"));
    EXPECT_EQ("hello%20world", to_id("http://example.com/base/dir/hello%20world"));

    // Files outside of the base URL throw
    EXPECT_THROW(to_id("http://example.com/base/dir"), RemoteCommsException);
    EXPECT_THROW(to_id("http://example.com/base/"), RemoteCommsException);
    EXPECT_THROW(to_id("https://example.com/base/dir/"), RemoteCommsException);
    EXPECT_THROW(to_id("http://www.example.com/base/dir/"), RemoteCommsException);
}

TEST(ItemId, make_child_id)
{
    EXPECT_EQ("foo", make_child_id(".", "foo"));
    EXPECT_EQ("foo/bar", make_child_id("foo", "bar"));
    EXPECT_EQ("foo/bar", make_child_id("foo/", "bar"));

    // Constructing a folder-type child ID
    EXPECT_EQ("foo/", make_child_id(".", "foo", true));
    EXPECT_EQ("foo/bar/", make_child_id("foo/", "bar", true));

    EXPECT_EQ("foo/hello%20world%3F", make_child_id("foo/", "hello world?"));
    EXPECT_EQ("foo/na%C3%AFve", make_child_id("foo/", "na\u00EFve"));

    EXPECT_THROW(make_child_id("foo/", "."), InvalidArgumentException);
    EXPECT_THROW(make_child_id("foo/", ".."), InvalidArgumentException);
}

TEST(ItemId, is_folder)
{
    EXPECT_TRUE(is_folder("."));
    EXPECT_TRUE(is_folder("foo/"));
    EXPECT_TRUE(is_folder("foo/bar/"));

    EXPECT_FALSE(is_folder("foo"));
    EXPECT_FALSE(is_folder("foo/bar"));

    EXPECT_THROW(is_folder(""), InvalidArgumentException);
}

int main(int argc, char**argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
