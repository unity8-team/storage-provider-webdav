#include "../../src/DavProvider.h"
#include <utils/DBusEnvironment.h>
#include <utils/DavEnvironment.h>
#include <utils/ProviderEnvironment.h>

#include <QCoreApplication>

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

protected:
    QUrl base_url(Context const& ctx) const override
    {
        Q_UNUSED(ctx);
        return base_url_;
    }

    void add_credentials(QNetworkRequest* request, Context const& ctx) const override
    {
        Q_UNUSED(request);
        Q_UNUSED(ctx);
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
                                2, *dbus_env_));
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
