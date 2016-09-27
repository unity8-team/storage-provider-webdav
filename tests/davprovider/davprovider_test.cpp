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

using namespace std;
using namespace unity::storage::provider;
using namespace unity::storage::qt::client;
using unity::storage::ItemType;

static constexpr int SIGNAL_WAIT_TIME = 30000;

class TestDavProvider : public DavProvider
{
public:
    TestDavProvider(QUrl const& base_url)
        : base_url_(base_url)
    {
    }

    QUrl base_url(Context const& ctx) const override
    {
        Q_UNUSED(ctx);
        return base_url_;
    }

    QNetworkReply *send_request(
        QNetworkRequest& request, QByteArray const& verb, QIODevice* data,
        unity::storage::provider::Context const& ctx) const override
    {
        const auto& creds = boost::get<PasswordCredentials>(ctx.credentials);
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
                                unique_ptr<ProviderBase>(new TestDavProvider(dav_env_->base_url())),
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

private:
    std::unique_ptr<DBusEnvironment> dbus_env_;
    std::unique_ptr<QTemporaryDir> tmp_dir_;
    std::unique_ptr<DavEnvironment> dav_env_;
    std::unique_ptr<ProviderEnvironment> provider_env_;
};

TEST_F(DavProviderTests, roots)
{
    auto account = get_client();
    auto future = account->roots();

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

int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
