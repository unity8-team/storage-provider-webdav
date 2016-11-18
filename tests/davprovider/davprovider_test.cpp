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
#include <unity/storage/qt/client/Downloader.h>
#include <unity/storage/qt/client/Exceptions.h>
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
using unity::storage::ConflictPolicy;
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
    "sunt in culpa qui officia deserunt mollit anim id est laborum.\n";

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
                                make_shared<TestDavProvider>(dav_env_->base_url()),
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

    void touch_file(string const& path)
    {
        string full_path = local_file(path);
        ASSERT_EQ(0, utimes(full_path.c_str(), nullptr));
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
    make_dir("folder");
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

    EXPECT_EQ("folder/", items[2]->native_identity());
    EXPECT_EQ(".", items[2]->parent_ids().at(0));
    EXPECT_EQ("folder", items[2]->name());
    EXPECT_EQ(ItemType::folder, items[2]->type());

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

TEST_F(DavProviderTests, metadata_not_found)
{
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

    shared_ptr<Item> item;
    {
        QFutureWatcher<shared_ptr<Item>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->get("foo.txt"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        try
        {
            watcher.result();
            FAIL();
        }
        catch (NotExistsException const& e)
        {
            EXPECT_TRUE(e.error_message().startsWith("Sabre\\DAV\\Exception\\NotFound: "))
                << e.error_message().toStdString();
        }
    }
}

TEST_F(DavProviderTests, create_folder)
{
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

    shared_ptr<Folder> folder;
    {
        QFutureWatcher<shared_ptr<Folder>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->create_folder("folder"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        folder = watcher.result();
    }

    EXPECT_EQ("folder/", folder->native_identity());
    EXPECT_EQ(".", folder->parent_ids().at(0));
    EXPECT_EQ("folder", folder->name());
    EXPECT_EQ(ItemType::folder, folder->type());
}

TEST_F(DavProviderTests, create_folder_overwrite_file)
{
    auto account = get_client();
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

    {
        QFutureWatcher<shared_ptr<Folder>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->create_folder("folder"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        try
        {
            watcher.result();
            FAIL();
        }
        catch (ExistsException const& e)
        {
            EXPECT_EQ("Sabre\\DAV\\Exception\\MethodNotAllowed: The resource you tried to create already exists", e.error_message());
        }
    }
}

TEST_F(DavProviderTests, create_folder_overwrite_folder)
{
    auto account = get_client();
    ASSERT_EQ(0, mkdir(local_file("folder").c_str(), 0755));

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

    {
        QFutureWatcher<shared_ptr<Folder>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->create_folder("folder"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        try
        {
            watcher.result();
            FAIL();
        }
        catch (ExistsException const& e)
        {
            EXPECT_EQ("Sabre\\DAV\\Exception\\MethodNotAllowed: The resource you tried to create already exists", e.error_message());
        }
    }
}

TEST_F(DavProviderTests, create_file)
{
    int const segments = 50;

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

TEST_F(DavProviderTests, create_file_over_existing_file)
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

    shared_ptr<Uploader> uploader;
    {
        QFutureWatcher<shared_ptr<Uploader>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->create_file("foo.txt", 0));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        uploader = watcher.result();
    }

    {
        QFutureWatcher<shared_ptr<File>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(uploader->finish_upload());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        try
        {
            watcher.result();
            FAIL();
        }
        catch (ConflictException const& e)
        {
            EXPECT_EQ("Sabre\\DAV\\Exception\\PreconditionFailed: An If-None-Match header was specified, but the ETag matched (or * was specified).", e.error_message());
        }
    }
}

#if 0
// v1 client API doesn't let us do this.  Revisit for v2 API.
TEST_F(DavProviderTests, create_file_overwrite_existing)
{
}
#endif

TEST_F(DavProviderTests, update)
{
    int const segments = 50;

    auto account = get_client();
    make_file("foo.txt");
    // Sleep to ensure modification time changes
    sleep(1);

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

    shared_ptr<File> file;
    {
        QFutureWatcher<shared_ptr<Item>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->get("foo.txt"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        file = dynamic_pointer_cast<File>(watcher.result());
    }
    ASSERT_NE(nullptr, file.get());
    QString old_etag = file->etag();

    shared_ptr<Uploader> uploader;
    {
        QFutureWatcher<shared_ptr<Uploader>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(file->create_uploader(ConflictPolicy::error_if_conflict, file_contents.size() * segments));
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
    file = watcher.result();

    EXPECT_NE(old_etag, file->etag());
    EXPECT_EQ(file_contents.size() * segments, file->size());
}

TEST_F(DavProviderTests, update_conflict)
{
    auto account = get_client();
    make_file("foo.txt");
    // Sleep to ensure modification time changes
    sleep(1);

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

    shared_ptr<File> file;
    {
        QFutureWatcher<shared_ptr<Item>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->get("foo.txt"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        file = dynamic_pointer_cast<File>(watcher.result());
    }
    ASSERT_NE(nullptr, file.get());

    // Change the file after metadata has been looked up
    touch_file("foo.txt");

    shared_ptr<Uploader> uploader;
    {
        QFutureWatcher<shared_ptr<Uploader>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(file->create_uploader(ConflictPolicy::error_if_conflict, 0));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        uploader = watcher.result();
    }

    {
        QFutureWatcher<shared_ptr<File>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(uploader->finish_upload());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        try
        {
            watcher.result();
            FAIL();
        }
        catch (ConflictException const& e)
        {
            EXPECT_EQ("Sabre\\DAV\\Exception\\PreconditionFailed: An If-Match header was specified, but none of the specified the ETags matched.", e.error_message());
        }
    }
}

TEST_F(DavProviderTests, upload_short_write)
{
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
        watcher.setFuture(root->create_file("foo.txt", 1000));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        uploader = watcher.result();
    }

    {
        QFutureWatcher<shared_ptr<File>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(uploader->finish_upload());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        try
        {
            watcher.result();
            FAIL();
        }
        catch (RemoteCommsException const& e)
        {
            EXPECT_EQ("Unknown error", e.error_message());
        }
    }
}

TEST_F(DavProviderTests, upload_cancel)
{
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
        watcher.setFuture(root->create_file("filename.txt", 1000));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        uploader = watcher.result();
    }

    {
        QFutureWatcher<void> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(uploader->cancel());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        // QFuture<void> doesn't have result, but exceptions will be
        // thrown from waitForFinished().
        watcher.waitForFinished();
    }
}

TEST_F(DavProviderTests, download)
{
    int const segments = 1000;
    string large_contents;
    large_contents.reserve(file_contents.size() * segments);
    for (int i = 0; i < segments; i++)
    {
        large_contents += file_contents;
    }
    string const full_path = local_file("foo.txt");
    {
        int fd = open(full_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
        ASSERT_GT(fd, 0);
        ASSERT_EQ(large_contents.size(), write(fd, &large_contents[0], large_contents.size())) << strerror(errno);
        ASSERT_EQ(0, close(fd));
    }

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

    shared_ptr<File> file;
    {
        QFutureWatcher<shared_ptr<Item>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->get("foo.txt"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        file = dynamic_pointer_cast<File>(watcher.result());
    }
    ASSERT_NE(nullptr, file.get());

    shared_ptr<Downloader> downloader;
    {
        QFutureWatcher<shared_ptr<Downloader>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(file->create_downloader());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        downloader = watcher.result();
    }

    int64_t n_read = 0;
    auto socket = downloader->socket();
    QObject::connect(socket.get(), &QIODevice::readyRead,
                     [socket, &large_contents, &n_read]() {
                         auto bytes = socket->readAll();
                         string const expected = large_contents.substr(
                             n_read, bytes.size());
                         EXPECT_EQ(expected, bytes.toStdString());
                         n_read += bytes.size();
                     });
    {
        QSignalSpy spy(socket.get(), &QIODevice::readChannelFinished);
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }

    {
        QFutureWatcher<void> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(downloader->finish_download());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        watcher.waitForFinished(); // to check for errors
    }

    EXPECT_EQ(large_contents.size(), n_read);
}

TEST_F(DavProviderTests, download_short_read)
{
    int const segments = 1000;
    {
        string full_path = local_file("foo.txt");
        int fd = open(full_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
        ASSERT_GT(fd, 0);
        for (int i = 0; i < segments; i++)
        {
            ASSERT_EQ(file_contents.size(), write(fd, &file_contents[0], file_contents.size())) << strerror(errno);
        }
        ASSERT_EQ(0, close(fd));
    }

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

    shared_ptr<File> file;
    {
        QFutureWatcher<shared_ptr<Item>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->get("foo.txt"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        file = dynamic_pointer_cast<File>(watcher.result());
    }
    ASSERT_NE(nullptr, file.get());

    shared_ptr<Downloader> downloader;
    {
        QFutureWatcher<shared_ptr<Downloader>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(file->create_downloader());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        downloader = watcher.result();
    }

    {
        QFutureWatcher<void> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(downloader->finish_download());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }

        try
        {
            watcher.waitForFinished(); // to check for errors
            FAIL();
        }
        catch (LogicException const& e)
        {
            EXPECT_EQ("finish called before all data sent", e.error_message());
        }
    }
}

TEST_F(DavProviderTests, download_not_found)
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

    shared_ptr<File> file;
    {
        QFutureWatcher<shared_ptr<Item>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(root->get("foo.txt"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        file = dynamic_pointer_cast<File>(watcher.result());
    }
    ASSERT_NE(nullptr, file.get());

    ASSERT_EQ(0, unlink(local_file("foo.txt").c_str()));

    shared_ptr<Downloader> downloader;
    {
        QFutureWatcher<shared_ptr<Downloader>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(file->create_downloader());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        downloader = watcher.result();
    }

    auto socket = downloader->socket();
    QObject::connect(socket.get(), &QIODevice::readyRead,
                     [socket]() {
                         socket->readAll();
                     });
    {
        QSignalSpy spy(socket.get(), &QIODevice::readChannelFinished);
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }

    {
        QFutureWatcher<void> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(downloader->finish_download());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }

        try
        {
            watcher.waitForFinished(); // to check for errors
            FAIL();
        }
        catch (NotExistsException const& e)
        {
            EXPECT_TRUE(e.error_message().startsWith("Sabre\\DAV\\Exception\\NotFound: "))
                << e.error_message().toStdString();
        }
    }
}

TEST_F(DavProviderTests, delete_item)
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

    {
        QFutureWatcher<void> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(item->delete_item());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        watcher.waitForFinished(); // to catch any errors
    }

    struct stat buf;
    EXPECT_EQ(-1, stat(local_file("foo.txt").c_str(), &buf));
    EXPECT_EQ(ENOENT, errno);
}

TEST_F(DavProviderTests, delete_item_not_found)
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

    ASSERT_EQ(0, unlink(local_file("foo.txt").c_str()));

    {
        QFutureWatcher<void> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(item->delete_item());
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        try
        {
            watcher.waitForFinished(); // to catch any errors
            FAIL();
        }
        catch (NotExistsException const& e)
        {
            EXPECT_TRUE(e.error_message().startsWith(
                            "Sabre\\DAV\\Exception\\NotFound: "))
                << e.error_message().toStdString();
        }
    }
}

TEST_F(DavProviderTests, move)
{
    string const full_path = local_file("foo.txt");
    {
        int fd = open(full_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
        ASSERT_GT(fd, 0);
        ASSERT_EQ(file_contents.size(), write(fd, &file_contents[0], file_contents.size())) << strerror(errno);
        ASSERT_EQ(0, close(fd));
    }

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

    {
        QFutureWatcher<shared_ptr<Item>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(item->move(root, "new-name.txt"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        item = watcher.result();
    }

    EXPECT_EQ("new-name.txt", item->native_identity());
    ASSERT_EQ(1, item->parent_ids().size());
    EXPECT_EQ(".", item->parent_ids().at(0));
    EXPECT_EQ("new-name.txt", item->name());
    EXPECT_NE(0, item->etag().size());
    EXPECT_EQ(ItemType::file, item->type());
    EXPECT_EQ(file_contents.size(), dynamic_pointer_cast<File>(item)->size());

    // The old file no longer exists
    struct stat buf;
    EXPECT_EQ(-1, stat(full_path.c_str(), &buf));
    EXPECT_EQ(ENOENT, errno);

    // And the new one does
    string const new_path = local_file("new-name.txt");
    EXPECT_EQ(0, stat(new_path.c_str(), &buf));
    EXPECT_EQ(file_contents.size(), buf.st_size);

    // And its has the expected contents
    {
        int fd = open(new_path.c_str(), O_RDONLY);
        ASSERT_GT(fd, 0);
        string contents(file_contents.size(), '\0');
        EXPECT_EQ(file_contents.size(), read(fd, &contents[0], contents.size()));
        EXPECT_EQ(0, close(fd));
        EXPECT_EQ(file_contents, contents);
    }
}

TEST_F(DavProviderTests, copy)
{
    string const full_path = local_file("foo.txt");
    {
        int fd = open(full_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
        ASSERT_GT(fd, 0);
        ASSERT_EQ(file_contents.size(), write(fd, &file_contents[0], file_contents.size())) << strerror(errno);
        ASSERT_EQ(0, close(fd));
    }

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

    {
        QFutureWatcher<shared_ptr<Item>> watcher;
        QSignalSpy spy(&watcher, &decltype(watcher)::finished);
        watcher.setFuture(item->copy(root, "new-name.txt"));
        if (spy.count() == 0)
        {
            ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
        }
        item = watcher.result();
    }

    EXPECT_EQ("new-name.txt", item->native_identity());
    ASSERT_EQ(1, item->parent_ids().size());
    EXPECT_EQ(".", item->parent_ids().at(0));
    EXPECT_EQ("new-name.txt", item->name());
    EXPECT_NE(0, item->etag().size());
    EXPECT_EQ(ItemType::file, item->type());
    EXPECT_EQ(file_contents.size(), dynamic_pointer_cast<File>(item)->size());

    // The old file still exists
    struct stat buf;
    EXPECT_EQ(0, stat(full_path.c_str(), &buf));

    // And the new one does too
    string const new_path = local_file("new-name.txt");
    EXPECT_EQ(0, stat(new_path.c_str(), &buf));
    EXPECT_EQ(file_contents.size(), buf.st_size);

    // And its has the expected contents
    {
        int fd = open(new_path.c_str(), O_RDONLY);
        ASSERT_GT(fd, 0);
        string contents(file_contents.size(), '\0');
        EXPECT_EQ(file_contents.size(), read(fd, &contents[0], contents.size()));
        EXPECT_EQ(0, close(fd));
        EXPECT_EQ(file_contents, contents);
    }
}

int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
