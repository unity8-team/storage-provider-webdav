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

#include "../../src/DavProvider.h"
#include <utils/DBusEnvironment.h>
#include <utils/DavEnvironment.h>
#include <utils/ProviderEnvironment.h>
#include <testsetup.h>

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <unity/storage/qt/Account.h>
#include <unity/storage/qt/Downloader.h>
#include <unity/storage/qt/Item.h>
#include <unity/storage/qt/ItemJob.h>
#include <unity/storage/qt/ItemListJob.h>
#include <unity/storage/qt/StorageError.h>
#include <unity/storage/qt/Uploader.h>
#include <unity/storage/qt/VoidJob.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <algorithm>

using namespace std;
using namespace unity::storage::qt;
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

    Account get_client() const
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
        ASSERT_EQ(0, utime(full_path.c_str(), nullptr));
    }

    void offset_mtime(string const& path, int offset_seconds)
    {
        string full_path = local_file(path);
        struct stat buf;
        ASSERT_EQ(0, stat(full_path.c_str(), &buf));
        struct utimbuf times;
        times.actime = buf.st_atime + offset_seconds;
        times.modtime = buf.st_mtime + offset_seconds;
        ASSERT_EQ(0, utime(full_path.c_str(), &times));
    }

private:
    std::unique_ptr<DBusEnvironment> dbus_env_;
    std::unique_ptr<QTemporaryDir> tmp_dir_;
    std::unique_ptr<DavEnvironment> dav_env_;
    std::unique_ptr<ProviderEnvironment> provider_env_;
};

namespace
{

template <typename Job>
void wait_for(Job* job)
{
    QSignalSpy spy(job, &Job::statusChanged);
    while (job->status() == Job::Loading)
    {
        if (!spy.wait(SIGNAL_WAIT_TIME))
        {
            throw runtime_error("Wait for statusChanged signal timed out");
        }
    }
}

QList<Item> get_items(ItemListJob *job)
{
    QList<Item> items;
    auto connection = QObject::connect(
        job, &ItemListJob::itemsReady,
        [&](QList<Item> const& new_items)
        {
            items.append(new_items);
        });
    try
    {
        wait_for(job);
    }
    catch (...)
    {
        QObject::disconnect(connection);
        throw;
    }
    QObject::disconnect(connection);
    return items;
}

Item get_root(Account const& account)
{
    unique_ptr<ItemListJob> job(account.roots());
    QList<Item> roots = get_items(job.get());
    if (job->status() != ItemListJob::Finished)
    {
        throw runtime_error("Account.roots(): " +
                            job->error().errorString().toStdString());
    }
    return roots.at(0);
}

}

TEST_F(DavProviderTests, roots)
{
    auto account = get_client();

    unique_ptr<ItemListJob> job(account.roots());
    QList<Item> roots = get_items(job.get());
    ASSERT_EQ(ItemListJob::Finished, job->status())
        << job->error().errorString().toStdString();

    ASSERT_EQ(1, roots.size());
    auto item = roots[0];
    EXPECT_EQ(".", item.itemId());
    EXPECT_EQ("Root", item.name());
    EXPECT_EQ(Item::Root, item.type());
    EXPECT_TRUE(item.parentIds().isEmpty());
    EXPECT_TRUE(item.lastModifiedTime().isValid());
}

TEST_F(DavProviderTests, list)
{
    auto account = get_client();
    make_file("foo.txt");
    make_file("bar.txt");
    make_dir("folder");
    make_file("I\u00F1t\u00EBrn\u00E2ti\u00F4n\u00E0liz\u00E6ti\u00F8n");

    Item root = get_root(account);

    unique_ptr<ItemListJob> job(root.list());
    QList<Item> items = get_items(job.get());
    ASSERT_EQ(ItemListJob::Finished, job->status())
        << job->error().errorString().toStdString();

    ASSERT_EQ(4, items.size());
    sort(items.begin(), items.end(),
         [](Item const& a, Item const& b) -> bool {
             return a.itemId() < b.itemId();
         });

    EXPECT_EQ("I%C3%B1t%C3%ABrn%C3%A2ti%C3%B4n%C3%A0liz%C3%A6ti%C3%B8n", items[0].itemId());
    EXPECT_EQ(".", items[0].parentIds().at(0));
    EXPECT_EQ("I\u00F1t\u00EBrn\u00E2ti\u00F4n\u00E0liz\u00E6ti\u00F8n", items[0].name());
    EXPECT_EQ(Item::File, items[0].type());

    EXPECT_EQ("bar.txt", items[1].itemId());
    EXPECT_EQ(".", items[1].parentIds().at(0));
    EXPECT_EQ("bar.txt", items[1].name());
    EXPECT_EQ(Item::File, items[1].type());

    EXPECT_EQ("folder/", items[2].itemId());
    EXPECT_EQ(".", items[2].parentIds().at(0));
    EXPECT_EQ("folder", items[2].name());
    EXPECT_EQ(Item::Folder, items[2].type());

    EXPECT_EQ("foo.txt", items[3].itemId());
    EXPECT_EQ(".", items[3].parentIds().at(0));
    EXPECT_EQ("foo.txt", items[3].name());
    EXPECT_EQ(Item::File, items[3].type());
}

TEST_F(DavProviderTests, list_on_file_fails)
{
    auto account = get_client();
    make_file("foo.txt");

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    Item item = job->item();
    unique_ptr<ItemListJob> list_job(item.list());
    QList<Item> items = get_items(list_job.get());
    ASSERT_EQ(ItemListJob::Error, list_job->status());

    auto error = list_job->error();
    EXPECT_EQ(StorageError::LogicError, error.type());
    // TODO: currently this call is being failed within the client
    // library rather than exercising the error path in the provider.
    // EXPECT_EQ("foo.txt is not a folder", error.message());
}

TEST_F(DavProviderTests, lookup)
{
    auto account = get_client();
    make_file("foo.txt");

    Item root = get_root(account);

    unique_ptr<ItemListJob> job(root.lookup("foo.txt"));
    QList<Item> items = get_items(job.get());
    ASSERT_EQ(ItemListJob::Finished, job->status())
        << job->error().errorString().toStdString();

    ASSERT_EQ(1, items.size());
    EXPECT_EQ("foo.txt", items[0].itemId());
    EXPECT_EQ(".", items[0].parentIds().at(0));
    EXPECT_EQ("foo.txt", items[0].name());
    EXPECT_EQ(Item::File, items[0].type());
}

TEST_F(DavProviderTests, metadata)
{
    auto account = get_client();
    make_file("foo.txt");

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    Item item = job->item();
    EXPECT_EQ("foo.txt", item.itemId());
    EXPECT_EQ(".", item.parentIds().at(0));
    EXPECT_EQ("foo.txt", item.name());
    EXPECT_EQ(Item::File, item.type());
}

TEST_F(DavProviderTests, metadata_not_found)
{
    auto account = get_client();

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Error, job->status());

    auto error = job->error();
    EXPECT_EQ(StorageError::NotExists, error.type());
    EXPECT_TRUE(error.message().startsWith("Sabre\\DAV\\Exception\\NotFound: "))
        << error.message().toStdString();
}

TEST_F(DavProviderTests, create_folder)
{
    auto account = get_client();

    Item root = get_root(account);
    unique_ptr<ItemJob> job(root.createFolder("folder"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    Item folder = job->item();
    EXPECT_EQ("folder/", folder.itemId());
    EXPECT_EQ(".", folder.parentIds().at(0));
    EXPECT_EQ("folder", folder.name());
    EXPECT_EQ(Item::Folder, folder.type());
}

TEST_F(DavProviderTests, create_folder_reserved_chars)
{
    auto account = get_client();

    Item root = get_root(account);
    unique_ptr<ItemJob> job(root.createFolder("14:19"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    Item folder = job->item();
    EXPECT_EQ("14:19/", folder.itemId());
    EXPECT_EQ(".", folder.parentIds().at(0));
    EXPECT_EQ("14:19", folder.name());
    EXPECT_EQ(Item::Folder, folder.type());
}

TEST_F(DavProviderTests, create_folder_overwrite_file)
{
    auto account = get_client();
    make_file("folder");

    Item root = get_root(account);
    unique_ptr<ItemJob> job(root.createFolder("folder"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Error, job->status());

    auto error = job->error();
    EXPECT_EQ(StorageError::Exists, error.type());
    EXPECT_EQ("Sabre\\DAV\\Exception\\MethodNotAllowed: The resource you tried to create already exists", error.message());
}

TEST_F(DavProviderTests, create_folder_overwrite_folder)
{
    auto account = get_client();
    ASSERT_EQ(0, mkdir(local_file("folder").c_str(), 0755));

    Item root = get_root(account);
    unique_ptr<ItemJob> job(root.createFolder("folder"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Error, job->status());

    auto error = job->error();
    EXPECT_EQ(StorageError::Exists, error.type());
    EXPECT_EQ("Sabre\\DAV\\Exception\\MethodNotAllowed: The resource you tried to create already exists", error.message());
}

TEST_F(DavProviderTests, create_file)
{
    int const segments = 50;

    auto account = get_client();
    Item root = get_root(account);

    unique_ptr<Uploader> uploader(
        root.createFile("filename.txt", Item::ErrorIfConflict,
                        file_contents.size() * segments, "text/plain"));

    int count = 0;
    QTimer timer;
    timer.setSingleShot(false);
    timer.setInterval(10);
    QObject::connect(&timer, &QTimer::timeout, [&] {
            uploader->write(&file_contents[0], file_contents.size());
            count++;
            if (count == segments)
            {
                uploader->close();
            }
        });

    QSignalSpy spy(uploader.get(), &Uploader::statusChanged);
    timer.start();
    while (uploader->status() == Uploader::Loading ||
           uploader->status() == Uploader::Ready)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }
    ASSERT_EQ(Uploader::Finished, uploader->status())
        << uploader->error().errorString().toStdString();

    auto file = uploader->item();
    EXPECT_EQ("filename.txt", file.itemId());
    ASSERT_EQ(1, file.parentIds().size());
    EXPECT_EQ(".", file.parentIds().at(0));
    EXPECT_EQ("filename.txt", file.name());
    EXPECT_NE(0, file.etag().size());
    EXPECT_EQ(Item::File, file.type());
    EXPECT_EQ(int64_t(file_contents.size() * segments), file.sizeInBytes());

    string full_path = local_file("filename.txt");
    struct stat buf;
    ASSERT_EQ(0, stat(full_path.c_str(), &buf));
    EXPECT_EQ(off_t(file_contents.size() * segments), buf.st_size);
}

TEST_F(DavProviderTests, create_file_over_existing_file)
{
    auto account = get_client();
    make_file("foo.txt");

    Item root = get_root(account);
    unique_ptr<Uploader> uploader(
        root.createFile("foo.txt", Item::ErrorIfConflict, 0, "text/plain"));
    QSignalSpy spy(uploader.get(), &Uploader::statusChanged);
    while (uploader->status() == Uploader::Loading)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }

    uploader->close();
    while (uploader->status() == Uploader::Ready)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }
    ASSERT_EQ(Uploader::Error, uploader->status());

    auto error = uploader->error();
    EXPECT_EQ(StorageError::Conflict, error.type());
    EXPECT_EQ("Sabre\\DAV\\Exception\\PreconditionFailed: An If-None-Match header was specified, but the ETag matched (or * was specified).", error.message());
}

TEST_F(DavProviderTests, create_file_overwrite_existing)
{
    auto account = get_client();
    make_file("foo.txt");

    Item root = get_root(account);
    unique_ptr<Uploader> uploader(
        root.createFile("foo.txt", Item::IgnoreConflict, 0, "text/plain"));
    QSignalSpy spy(uploader.get(), &Uploader::statusChanged);
    while (uploader->status() == Uploader::Loading)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }

    uploader->close();
    while (uploader->status() == Uploader::Ready)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }
    ASSERT_EQ(Uploader::Finished, uploader->status())
        << uploader->error().errorString().toStdString();
}

TEST_F(DavProviderTests, update)
{
    int const segments = 50;

    auto account = get_client();
    make_file("foo.txt");
    // Offset to ensure modification time changes
    offset_mtime("foo.txt", -10);

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    auto file = job->item();
    QString old_etag = file.etag();

    unique_ptr<Uploader> uploader(
        file.createUploader(Item::ErrorIfConflict,
                            file_contents.size() * segments));

    int count = 0;
    QTimer timer;
    timer.setSingleShot(false);
    timer.setInterval(10);
    QObject::connect(&timer, &QTimer::timeout, [&] {
            uploader->write(&file_contents[0], file_contents.size());
            count++;
            if (count == segments)
            {
                uploader->close();
            }
        });

    QSignalSpy spy(uploader.get(), &Uploader::statusChanged);
    timer.start();
    while (uploader->status() == Uploader::Loading ||
           uploader->status() == Uploader::Ready)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }
    ASSERT_EQ(Uploader::Finished, uploader->status())
        << uploader->error().errorString().toStdString();

    file = uploader->item();
    EXPECT_NE(old_etag, file.etag());
    EXPECT_EQ(int64_t(file_contents.size() * segments), file.sizeInBytes());
}

TEST_F(DavProviderTests, update_conflict)
{
    auto account = get_client();
    make_file("foo.txt");
    // Offset to ensure modification time changes
    offset_mtime("foo.txt", -10);

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    // Change the file after metadata has been looked up
    touch_file("foo.txt");

    auto file = job->item();
    unique_ptr<Uploader> uploader(
        file.createUploader(Item::ErrorIfConflict, 0));
    QSignalSpy spy(uploader.get(), &Uploader::statusChanged);
    while (uploader->status() == Uploader::Loading)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }

    uploader->close();
    while (uploader->status() == Uploader::Ready)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }
    ASSERT_EQ(Uploader::Error, uploader->status());

    auto error = uploader->error();
    EXPECT_EQ(StorageError::Conflict, error.type());
    EXPECT_EQ("Sabre\\DAV\\Exception\\PreconditionFailed: An If-Match header was specified, but none of the specified the ETags matched.", error.message());
}

TEST_F(DavProviderTests, upload_short_write)
{
    auto account = get_client();

    Item root = get_root(account);
    unique_ptr<Uploader> uploader(
        root.createFile("foo.txt", Item::ErrorIfConflict, 1000, "text/plain"));
    QSignalSpy spy(uploader.get(), &Uploader::statusChanged);
    while (uploader->status() == Uploader::Loading)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }

    uploader->close();
    while (uploader->status() == Uploader::Ready)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }
    ASSERT_EQ(Uploader::Error, uploader->status());

    auto error = uploader->error();
    EXPECT_EQ(StorageError::RemoteCommsError, error.type());
    EXPECT_EQ("Unknown error", error.message());
}

TEST_F(DavProviderTests, upload_cancel)
{
    auto account = get_client();

    Item root = get_root(account);
    unique_ptr<Uploader> uploader(
        root.createFile("foo.txt", Item::ErrorIfConflict, 1000, "text/plain"));
    QSignalSpy spy(uploader.get(), &Uploader::statusChanged);
    while (uploader->status() == Uploader::Loading)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }

    uploader->cancel();
    while (uploader->status() == Uploader::Ready)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }
    ASSERT_EQ(Uploader::Cancelled, uploader->status());

    auto error = uploader->error();
    EXPECT_EQ(StorageError::Cancelled, error.type());
    EXPECT_EQ("Uploader::cancel(): upload was cancelled", error.message());
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
        ASSERT_EQ(ssize_t(large_contents.size()), write(fd, &large_contents[0], large_contents.size())) << strerror(errno);
        ASSERT_EQ(0, close(fd));
    }

    auto account = get_client();

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    auto file = job->item();

    unique_ptr<Downloader> downloader(
        file.createDownloader(Item::ErrorIfConflict));

    int64_t n_read = 0;
    QObject::connect(downloader.get(), &QIODevice::readyRead,
                     [&]() {
                         auto bytes = downloader->readAll();
                         string const expected = large_contents.substr(
                             n_read, bytes.size());
                         EXPECT_EQ(expected, bytes.toStdString());
                         n_read += bytes.size();
                     });
    QSignalSpy read_finished_spy(
        downloader.get(), &QIODevice::readChannelFinished);
    ASSERT_TRUE(read_finished_spy.wait(SIGNAL_WAIT_TIME));

    QSignalSpy status_spy(downloader.get(), &Downloader::statusChanged);
    downloader->close();
    while (downloader->status() == Downloader::Ready)
    {
        ASSERT_TRUE(status_spy.wait(SIGNAL_WAIT_TIME));
    }
    ASSERT_EQ(Downloader::Finished, downloader->status())
        << downloader->error().errorString().toStdString();

    EXPECT_EQ(int64_t(large_contents.size()), n_read);
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
            ASSERT_EQ(ssize_t(file_contents.size()), write(fd, &file_contents[0], file_contents.size())) << strerror(errno);
        }
        ASSERT_EQ(0, close(fd));
    }

    auto account = get_client();

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    auto file = job->item();
    unique_ptr<Downloader> downloader(
        file.createDownloader(Item::ErrorIfConflict));

    QSignalSpy spy(downloader.get(), &Downloader::statusChanged);
    while (downloader->status() == Downloader::Loading)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }

    downloader->close();
    while (downloader->status() == Downloader::Ready)
    {
        ASSERT_TRUE(spy.wait(SIGNAL_WAIT_TIME));
    }
    ASSERT_EQ(Downloader::Error, downloader->status());

    auto error = downloader->error();
    EXPECT_EQ(StorageError::LogicError, error.type());
    EXPECT_EQ("finish called before all data sent", error.message());
}

TEST_F(DavProviderTests, download_not_found)
{
    auto account = get_client();
    make_file("foo.txt");

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    auto file = job->item();

    ASSERT_EQ(0, unlink(local_file("foo.txt").c_str()));

    unique_ptr<Downloader> downloader(
        file.createDownloader(Item::ErrorIfConflict));

    QObject::connect(downloader.get(), &QIODevice::readyRead,
                     [&]() {
                         downloader->readAll();
                     });
    QSignalSpy read_finished_spy(
        downloader.get(), &QIODevice::readChannelFinished);
    ASSERT_TRUE(read_finished_spy.wait(SIGNAL_WAIT_TIME));

    QSignalSpy status_spy(downloader.get(), &Downloader::statusChanged);
    downloader->close();
    while (downloader->status() == Downloader::Ready)
    {
        ASSERT_TRUE(status_spy.wait(SIGNAL_WAIT_TIME));
    }
    ASSERT_EQ(Downloader::Error, downloader->status());

    auto error = downloader->error();
    EXPECT_EQ(StorageError::NotExists, error.type());
    EXPECT_TRUE(error.message().startsWith("Sabre\\DAV\\Exception\\NotFound: "))
        << error.message().toStdString();
}

TEST_F(DavProviderTests, delete_item)
{
    auto account = get_client();
    make_file("foo.txt");

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    Item item = job->item();
    unique_ptr<VoidJob> delete_job(item.deleteItem());
    wait_for(delete_job.get());
    ASSERT_EQ(VoidJob::Finished, delete_job->status())
        << delete_job->error().errorString().toStdString();

    struct stat buf;
    EXPECT_EQ(-1, stat(local_file("foo.txt").c_str(), &buf));
    EXPECT_EQ(ENOENT, errno);
}

TEST_F(DavProviderTests, delete_item_not_found)
{
    auto account = get_client();
    make_file("foo.txt");

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    Item item = job->item();

    ASSERT_EQ(0, unlink(local_file("foo.txt").c_str()));

    unique_ptr<VoidJob> delete_job(item.deleteItem());
    wait_for(delete_job.get());
    ASSERT_EQ(VoidJob::Error, delete_job->status());

    auto error = delete_job->error();
    EXPECT_EQ(StorageError::NotExists, error.type());
    EXPECT_TRUE(error.message().startsWith("Sabre\\DAV\\Exception\\NotFound: "))
        << error.message().toStdString();
}

TEST_F(DavProviderTests, move)
{
    string const full_path = local_file("foo.txt");
    {
        int fd = open(full_path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
        ASSERT_GT(fd, 0);
        ASSERT_EQ(ssize_t(file_contents.size()), write(fd, &file_contents[0], file_contents.size())) << strerror(errno);
        ASSERT_EQ(0, close(fd));
    }

    auto account = get_client();
    Item root = get_root(account);

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    Item item = job->item();

    job.reset(item.move(root, "new-name.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    item = job->item();
    EXPECT_EQ("new-name.txt", item.itemId());
    ASSERT_EQ(1, item.parentIds().size());
    EXPECT_EQ(".", item.parentIds().at(0));
    EXPECT_EQ("new-name.txt", item.name());
    EXPECT_NE(0, item.etag().size());
    EXPECT_EQ(Item::File, item.type());
    EXPECT_EQ(int64_t(file_contents.size()), item.sizeInBytes());

    // The old file no longer exists
    struct stat buf;
    EXPECT_EQ(-1, stat(full_path.c_str(), &buf));
    EXPECT_EQ(ENOENT, errno);

    // And the new one does
    string const new_path = local_file("new-name.txt");
    EXPECT_EQ(0, stat(new_path.c_str(), &buf));
    EXPECT_EQ(off_t(file_contents.size()), buf.st_size);

    // And its has the expected contents
    {
        int fd = open(new_path.c_str(), O_RDONLY);
        ASSERT_GT(fd, 0);
        string contents(file_contents.size(), '\0');
        EXPECT_EQ(ssize_t(file_contents.size()), read(fd, &contents[0], contents.size()));
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
        ASSERT_EQ(ssize_t(file_contents.size()), write(fd, &file_contents[0], file_contents.size())) << strerror(errno);
        ASSERT_EQ(0, close(fd));
    }

    auto account = get_client();
    Item root = get_root(account);

    unique_ptr<ItemJob> job(account.get("foo.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    Item item = job->item();

    job.reset(item.copy(root, "new-name.txt"));
    wait_for(job.get());
    ASSERT_EQ(ItemJob::Finished, job->status())
        << job->error().errorString().toStdString();

    item = job->item();
    EXPECT_EQ("new-name.txt", item.itemId());
    ASSERT_EQ(1, item.parentIds().size());
    EXPECT_EQ(".", item.parentIds().at(0));
    EXPECT_EQ("new-name.txt", item.name());
    EXPECT_NE(0, item.etag().size());
    EXPECT_EQ(Item::File, item.type());
    EXPECT_EQ(int64_t(file_contents.size()), item.sizeInBytes());

    // The old file still exists
    struct stat buf;
    EXPECT_EQ(0, stat(full_path.c_str(), &buf));

    // And the new one does too
    string const new_path = local_file("new-name.txt");
    EXPECT_EQ(0, stat(new_path.c_str(), &buf));
    EXPECT_EQ(off_t(file_contents.size()), buf.st_size);

    // And its has the expected contents
    {
        int fd = open(new_path.c_str(), O_RDONLY);
        ASSERT_GT(fd, 0);
        string contents(file_contents.size(), '\0');
        EXPECT_EQ(ssize_t(file_contents.size()), read(fd, &contents[0], contents.size()));
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
