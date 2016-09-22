#include "../../src/DavProvider.h"
#include <utils/DBusEnvironment.h>
#include <utils/DavEnvironment.h>
#include <utils/ProviderEnvironment.h>

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

#include <gtest/gtest.h>

using namespace std;
using namespace unity::storage::provider;

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
        dav_env_.reset(new DavEnvironment("/tmp"));
        provider_env_.reset(new ProviderEnvironment(
                                unique_ptr<ProviderBase>(new TestDavProvider(dav_env_->base_url())),
                                3, *dbus_env_));
    }

    void TearDown() override
    {
        provider_env_.reset();
        dav_env_.reset();
        dbus_env_.reset();
    }

private:
    std::unique_ptr<DBusEnvironment> dbus_env_;
    std::unique_ptr<DavEnvironment> dav_env_;
    std::unique_ptr<ProviderEnvironment> provider_env_;
};

TEST_F(DavProviderTests, roots)
{
}

int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
