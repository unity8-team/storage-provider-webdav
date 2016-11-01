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
#include <QTimer>
#include <unity/storage/qt/client/Account.h>
#include <unity/storage/qt/client/Root.h>
#include <unity/storage/qt/client/Uploader.h>

#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>

using namespace std;
using namespace unity::storage::qt::client;
using unity::storage::ItemType;
namespace provider = unity::storage::provider;

void PrintTo(QString const& str, std::ostream* os)
{
    *os << "QString(\"" << str.toStdString() << "\")";
}

namespace
{

const string file_contents =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
    "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
    "enim ad minim veniam, quis nostrud exercitation ullamco laboris "
    "nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor "
    "in reprehenderit in voluptate velit esse cillum dolore eu fugiat "
    "nulla pariatur. Excepteur sint occaecat cupidatat non proident, "
    "sunt in culpa qui officia deserunt mollit anim id est laborum.";

constexpr int SIGNAL_WAIT_TIME = 30000;

}

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
        const auto credentials = QByteArray::fromStdString(creds.username + ":" +
                                                           creds.password);
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
                                1, *dbus_env_));
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

    string local_file(string const& path)
    {
        return tmp_dir_->path().toStdString() + "/" + path;
    }

    void make_file(string const& path)
    {
        string full_path = local_file(path);
        int fd = open(full_path.c_str(), O_CREAT | O_EXCL, 0644);
        ASSERT_GT(fd, 0);
        ASSERT_EQ(0, close(fd));
    }

    void make_dir(string const& path)
    {
        string full_path = local_file(path);
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
    //make_dir("folder");
    make_file("folder");
    make_file("I\u00F1t\u00EBrn\u00E2ti\u00F4n\u00E0liz\u00E6ti\u00F8n");

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
    ASSERT_EQ(4, items.size());
    sort(items.begin(), items.end(),
         [](shared_ptr<Item> const& a, shared_ptr<Item> const& b) -> bool {
             return a->native_identity() < b->native_identity();
         });

    EXPECT_EQ("I%C3%B1t%C3%ABrn%C3%A2ti%C3%B4n%C3%A0liz%C3%A6ti%C3%B8n", items[0]->native_identity());
    EXPECT_EQ(".", items[0]->parent_ids().at(0));
    EXPECT_EQ("I\u00F1t\u00EBrn\u00E2ti\u00F4n\u00E0liz\u00E6ti\u00F8n", items[0]->name());
    EXPECT_EQ(ItemType::file, items[0]->type());

    EXPECT_EQ("bar.txt", items[1]->native_identity());
    EXPECT_EQ(".", items[1]->parent_ids().at(0));
    EXPECT_EQ("bar.txt", items[1]->name());
    EXPECT_EQ(ItemType::file, items[1]->type());

    // FIXME: SabreDAV doesn't provide an ETag for folders, which
    // currently trips up storage-framework's client side metadata
    // validation.

    //EXPECT_EQ("folder/", items[2]->native_identity());
    EXPECT_EQ("folder", items[2]->native_identity());
    EXPECT_EQ(".", items[2]->parent_ids().at(0));
    EXPECT_EQ("folder", items[2]->name());
    //EXPECT_EQ(ItemType::folder, items[2]->type());
    EXPECT_EQ(ItemType::file, items[2]->type());

    EXPECT_EQ("foo.txt", items[3]->native_identity());
    EXPECT_EQ(".", items[3]->parent_ids().at(0));
    EXPECT_EQ("foo.txt", items[3]->name());
    EXPECT_EQ(ItemType::file, items[3]->type());
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
    EXPECT_EQ(".", items[0]->parent_ids().at(0));
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
    EXPECT_EQ(".", item->parent_ids().at(0));
    EXPECT_EQ("foo.txt", item->name());
    EXPECT_EQ(ItemType::file, item->type());
}

TEST_F(DavProviderTests, create_file)
{
    const int segments = 50;

    auto account = get_client();
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

    shared_ptr<Uploader> uploader;
    {
        QFutureWatcher<shared_ptr<Uploader>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->create_file("filename.txt", file_contents.size() * segments));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        uploader = watcher.result();
    }

    auto socket = uploader->socket();
    int count = 0;
    QTimer timer;
    timer.setSingleShot(false);
    timer.setInterval(10);
    QFutureWatcher<shared_ptr<File>> watcher;
    QObject::connect(&timer, &QTimer::timeout, [&] {
            socket->write(&file_contents[0], file_contents.size());
            count++;
            if (count == segments)
            {
                watcher.setFuture(uploader->finish_upload());
            }
        });

    QSignalSpy spy(&watcher, &decltype(watcher)::finished);
    timer.start();
    ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    auto file = watcher.result();

    EXPECT_EQ("filename.txt", file->native_identity());
    ASSERT_EQ(1, file->parent_ids().size());
    EXPECT_EQ(".", file->parent_ids().at(0));
    EXPECT_EQ("filename.txt", file->name());
    EXPECT_NE(0, file->etag().size());
    EXPECT_EQ(ItemType::file, file->type());
    EXPECT_EQ(file_contents.size() * segments, file->size());

    string full_path = local_file("filename.txt");
    struct stat buf;
    ASSERT_EQ(0, stat(full_path.c_str(), &buf));
    EXPECT_EQ(file_contents.size() * segments, buf.st_size);
}

int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
