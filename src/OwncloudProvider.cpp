#include "OwncloudProvider.h"

#include <QUrl>

using namespace std;
using namespace unity::storage::provider;

OwncloudProvider::OwncloudProvider()
{
}

OwncloudProvider::~OwncloudProvider() = default;

QUrl OwncloudProvider::base_url(Context const& ctx) const
{
    return QUrl("http://whatever.com/");
}

void OwncloudProvider::add_credentials(QNetworkRequest *request,
                                       Context const& ctx) const
{
}
