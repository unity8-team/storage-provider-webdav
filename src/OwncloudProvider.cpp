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

QNetworkReply *OwncloudProvider::send_request(
    QNetworkRequest& request, QByteArray const& verb, QIODevice* data,
    Context const& ctx) const
{
}
