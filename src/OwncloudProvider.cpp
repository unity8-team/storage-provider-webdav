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
    const auto& creds = boost::get<PasswordCredentials>(ctx.credentials);
    return QUrl(QStringLiteral("http://localhost:8888/owncloud/remote.php/dav/files/%1/").arg(QString::fromStdString(creds.username)));
}

QNetworkReply *OwncloudProvider::send_request(
    QNetworkRequest& request, QByteArray const& verb, QIODevice* data,
    Context const& ctx) const
{
    Q_UNUSED(ctx);

    const auto& creds = boost::get<PasswordCredentials>(ctx.credentials);
    const auto credentials = QByteArray::fromStdString(creds.username + ":" +
                                                       creds.password);
    request.setRawHeader(QByteArrayLiteral("Authorization"),
                         QByteArrayLiteral("Basic ") + credentials.toBase64());
    printf("Sending request to %s with credentials %s\n",
           request.url().toEncoded().constData(), credentials.constData());
    QNetworkReply *reply = network_->sendCustomRequest(request, verb, data);
    return reply;
}
