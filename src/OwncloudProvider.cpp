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
