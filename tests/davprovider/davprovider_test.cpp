#include "../../src/DavProvider.h"
#include <utils/DBusEnvironment.h>
#include <utils/DavEnvironment.h>
#include <utils/ProviderEnvironment.h>
#include <testsetup.h>

#include <QCoreApplication>
#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <unity/storage/qt/client/Account.h>
#include <unity/storage/qt/client/Root.h>

#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;
using namespace unity::storage::qt::client;
using unity::storage::ItemType;
namespace provider = unity::storage::provider;

static constexpr int SIGNAL_WAIT_TIME = 30000;

class TestDavProvider : public DavProvider
{
public:
    TestDavProvider(QUrl const& base_url)
        : base_url_(base_url)
    {
    }

    QUrl base_url(provider::Context const& ctx) const override
    {
        Q_UNUSED(ctx);
        return base_url_;
    }

    QNetworkReply *send_request(
        QNetworkRequest& request, QByteArray const& verb, QIODevice* data,
        provider::Context const& ctx) const override
    {
        const auto& creds = boost::get<provider::PasswordCredentials>(ctx.credentials);
        const auto credentials = QByteArray::fromStdString(creds.username + ":" + creds.password);;
        request.setRawHeader(QByteArrayLiteral("Authorization"),
                             QByteArrayLiteral("Basic ") + credentials.toBase64());
        return network_->sendCustomRequest(request, verb, data);
    }

private:
    QUrl const base_url_;
};

class DavProviderTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        dbus_env_.reset(new DBusEnvironment);
        dbus_env_->start_services();

        tmp_dir_.reset(new QTemporaryDir(TEST_BIN_DIR "/dav-test.XXXXXX"));
        ASSERT_TRUE(tmp_dir_->isValid());

        dav_env_.reset(new DavEnvironment(tmp_dir_->path()));
        provider_env_.reset(new ProviderEnvironment(
                                unique_ptr<provider::ProviderBase>(new TestDavProvider(dav_env_->base_url())),
                                3, *dbus_env_));
    }

    void TearDown() override
    {
        provider_env_.reset();
        dav_env_.reset();
        tmp_dir_.reset();
        dbus_env_.reset();
    }

    shared_ptr<Account> get_client() const
    {
        return provider_env_->get_client();
    }

    void make_file(string const& path)
    {
        string full_path = tmp_dir_->path().toStdString() + "/" + path;
        int fd = open(full_path.c_str(), O_CREAT | O_EXCL, 0644);
        ASSERT_GT(fd, 0);
        ASSERT_EQ(0, close(fd));
    }

    void make_dir(string const& path)
    {
        string full_path = tmp_dir_->path().toStdString() + "/" + path;
        ASSERT_EQ(0, mkdir(full_path.c_str(), 0755));
    }

private:
    std::unique_ptr<DBusEnvironment> dbus_env_;
    std::unique_ptr<QTemporaryDir> tmp_dir_;
    std::unique_ptr<DavEnvironment> dav_env_;
    std::unique_ptr<ProviderEnvironment> provider_env_;
};

TEST_F(DavProviderTests, roots)
{
    auto account = get_client();

    QFutureWatcher<QVector<shared_ptr<Root>>> watcher;
    QSignalSpy spy(&watcher, &decltype(watcher)::finished);
    watcher.setFuture(account->roots());
    if (spy.count() == 0)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }

    auto roots = watcher.result();
    ASSERT_EQ(1, roots.size());
    auto item = roots[0];
    EXPECT_EQ(".", item->native_identity());
    EXPECT_EQ("Root", item->name());
    EXPECT_EQ(ItemType::root, item->type());
    EXPECT_TRUE(item->parent_ids().isEmpty());
    EXPECT_TRUE(item->last_modified_time().isValid());
}

TEST_F(DavProviderTests, list)
{
    auto account = get_client();
    make_file("foo.txt");
    make_file("bar.txt");
    make_file("folder");

    shared_ptr<Root> root;
    {
        QFutureWatcher<QVector<shared_ptr<Root>>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(account->roots());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        auto roots = watcher.result();
        ASSERT_EQ(1, roots.size());
        root = roots[0];
    }

    vector<shared_ptr<Item>> items;
    {
        QFutureWatcher<QVector<shared_ptr<Item>>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->list());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        auto res = watcher.result();
        items.assign(res.begin(), res.end());
    }
    EXPECT_EQ(3, items.size());
}

TEST_F(DavProviderTests, lookup)
{
    auto account = get_client();
    make_file("foo.txt");

    shared_ptr<Root> root;
    {
        QFutureWatcher<QVector<shared_ptr<Root>>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(account->roots());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        auto roots = watcher.result();
        ASSERT_EQ(1, roots.size());
        root = roots[0];
    }

    vector<shared_ptr<Item>> items;
    {
        QFutureWatcher<QVector<shared_ptr<Item>>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->lookup("foo.txt"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        auto res = watcher.result();
        items.assign(res.begin(), res.end());
    }
    ASSERT_EQ(1, items.size());
    EXPECT_EQ("foo.txt", items[0]->native_identity());
    ASSERT_EQ(1, items[0]->parent_ids().size());
    EXPECT_EQ(".", items[0]->parent_ids()[0]);
    EXPECT_EQ("foo.txt", items[0]->name());
    EXPECT_EQ(ItemType::file, items[0]->type());
}

TEST_F(DavProviderTests, metadata)
{
    auto account = get_client();
    make_file("foo.txt");

    shared_ptr<Root> root;
    {
        QFutureWatcher<QVector<shared_ptr<Root>>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(account->roots());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        auto roots = watcher.result();
        ASSERT_EQ(1, roots.size());
        root = roots[0];
    }

    shared_ptr<Item> item;
    {
        QFutureWatcher<shared_ptr<Item>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->get("foo.txt"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        item = watcher.result();
    }

    EXPECT_EQ("foo.txt", item->native_identity());
    ASSERT_EQ(1, item->parent_ids().size());
    EXPECT_EQ(".", item->parent_ids()[0]);
    EXPECT_EQ("foo.txt", item->name());
    EXPECT_EQ(ItemType::file, item->type());
}

int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
