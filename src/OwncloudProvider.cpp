#include "OwncloudProvider.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>

using namespace std;
using namespace unity::storage::provider;

OwncloudProvider::OwncloudProvider()
{
}

OwncloudProvider::~OwncloudProvider() = default;

QUrl OwncloudProvider::base_url(Context const& ctx) const
{
    Q_UNUSED(ctx);
    return QUrl("http://localhost:8080/");
}

QNetworkReply *OwncloudProvider::send_request(
    QNetworkRequest& request, QByteArray const& verb, QIODevice* data,
    Context const& ctx) const
{
    Q_UNUSED(ctx);

    const auto credentials = QByteArrayLiteral("user:pass");
    request.setRawHeader(QByteArrayLiteral("Authorization"),
                         QByteArrayLiteral("Basic ") + credentials.toBase64());
    QNetworkReply *reply = network_->sendCustomRequest(request, verb, data);
    return reply;
}
