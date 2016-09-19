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


int main(int argc, char**argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
